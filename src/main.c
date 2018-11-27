#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdbool.h>

#include "messages.h"
#include "data_structures.h"
#include "client_update.h"
#include "global_ordering.h"
#include "leader_election.h"
#include "logging.h"
#include "multicast.h"
#include "prepare.h"
#include "serialize.h"
#include "update_globals.h"

int LOG_LEVEL = DEBUG; // set log level to debug for logging
int MAX_CLIENT_ID = 32; // size of arrays that are indexed by client id
                        // updated by increase_array_size()

int LEADER_ELECTION = 1;
int REG_LEADER = 2;
int REG_NONLEADER = 3;


// TODO Need to have a data structure probably so that you can tell when a client update
// has been bound to a specific sequence number

// server state variables
int MY_SERVER_ID; // unique identifier for this server
int NUM_PEERS;    // how many peers in the protocol
char *PORT;       // what port is it listening on
char **PEERS;     // who all the list of peers
int STATE;        // one of {LEADER_ELECTION, REG_LEADER, REG_NONLEADER}
                  // defined in data_structures.h

// view state variables
int LAST_ATTEMPTED; // last view this server attempted to install
int LAST_INSTALLED; // last  view this server installed
View_Change **VC;   // array of View_Change messages, indexed by server_id
                    // size of NUM_PEERS

// prepare phase variables
Prepare *PREPARED;      // the Prepare message from last preinstalled view, if any
Prepare **PREPARE_OKS;  // array of Prepare_OK messages received, indexed by serv id
                        // size of NUM_PEERS

// global ordering variables
int LOCAL_ARU;                 // local aru value of this server
int LAST_PROPOSED;             // last sequence number proposed by the leader
Global_Slot **GLOBAL_HISTORY;  // array of global_slots indexed by sequence number
                               // size is dynamic by increase_array_size()
                               // defined in data_structures.h

// timer variables
struct timeval PROGRESS_TIMER; // timeout on making global progress
bool PROGRESS_TIMER_SET;       // keep track of whether the progress time was set
uint32_t PROGRESS_TIMEOUT;     // how long to wait until PROGRESS_TIMER is timed out
uint32_t *UPDATE_TIMER;       // array of timeouts, indexed by client id
                               // dynamic size by increase_array_size()
uint32_t UPDATE_TIMEOUT;       // how long to wait until UPDATE_TIMER is timed out

// client handling variables
node_t *UPDATE_QUEUE;             // queue of Client_Update messages
uint32_t *LAST_EXECUTED;          // array of timestamps, indexed by client_id
uint32_t *LAST_ENQUEUED;          // array of timestamps, indexed by client_id
                                  // stores tv_sec field in struct timeval
                                  // dynamic size
Client_Update **PENDING_UPDATES;  // array of Client_Update messages, indexed by client_id
                                  // dynamic size

// when initializing node_t objects (really all of the structs)
// need to explicitly set the pointers to null and make sure that all the code
// expects those to be null not zero
void initialize_globals()
{
    STATE = LEADER_ELECTION; // when we start we probably want to elect a leader
    LAST_ATTEMPTED = 0;
    LAST_INSTALLED = 0;

    VC = malloc(sizeof(View_Change *) * NUM_PEERS);
    memset(VC, 0, sizeof(View_Change *) * NUM_PEERS);

    PREPARED = NULL;

    PREPARE_OKS = malloc(sizeof(Prepare_OK *) * NUM_PEERS);
    memset(PREPARE_OKS, 0, sizeof(Prepare_OK *) * NUM_PEERS);

    LOCAL_ARU = 0;
    LAST_PROPOSED = 0;

    GLOBAL_HISTORY = malloc(sizeof(Global_Slot *) * MAX_CLIENT_ID);
    memset(GLOBAL_HISTORY, 0, sizeof(Global_Slot *) * MAX_CLIENT_ID);

    PROGRESS_TIMER_SET = false;
    PROGRESS_TIMEOUT = 2; // how about setting to 2 sec for first timeout
    UPDATE_TIMEOUT = 2;

    UPDATE_TIMER = malloc(sizeof(uint32_t) * MAX_CLIENT_ID);
    memset(UPDATE_TIMER, 0, sizeof(uint32_t) * MAX_CLIENT_ID);

    UPDATE_QUEUE = malloc(sizeof(node_t));
    UPDATE_QUEUE->data = NULL;
    UPDATE_QUEUE->next = NULL;
    UPDATE_QUEUE->data_type = 0;

    LAST_EXECUTED = malloc(sizeof(uint32_t) * MAX_CLIENT_ID);
    memset(LAST_EXECUTED, 0, sizeof(uint32_t) * MAX_CLIENT_ID);
    LAST_ENQUEUED = malloc(sizeof(uint32_t) * MAX_CLIENT_ID);
    memset(LAST_ENQUEUED, 0, sizeof(uint32_t) * MAX_CLIENT_ID);
    PENDING_UPDATES = malloc(sizeof(Client_Update *) * MAX_CLIENT_ID);
    memset(PENDING_UPDATES, 0, sizeof(Client_Update *) * MAX_CLIENT_ID);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

char **open_parse_hostfile(char *hostfile)
{
    char hostname[64];
    int cur_hostname = gethostname(hostname, 63);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Got hostname: %s\n", hostname);
    if (cur_hostname == -1)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "Could not get hostname\n");
        exit(1);
    }

    FILE *fp = fopen(hostfile, "r");
    if (fp == NULL)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "Could not open hostfile\n");
        exit(1);
    }
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Opened hostfile\n");

    // get number of lines in hostfile
    int numLines = 0;
    char chr;
    chr = getc(fp);
    while (chr != EOF)
    {
        if (chr == '\n')
            numLines++;
        chr = getc(fp);
    }
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Hostfile is %d lines\n", numLines);
    fseek(fp, 0, SEEK_SET); // reset to beginning

    // malloc 2-D array of hostnames
    char **hosts = malloc(numLines * sizeof(char *));
    // keep track of size of host file
    NUM_PEERS = numLines;
    if (hosts == NULL)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "Could not malloc hosts array\n");
        exit(1);
    }

    // read in all the host names
    int i = 0;
    for (i = 0; i < NUM_PEERS; i++)
    {
        hosts[i] = (char *)malloc(255);
        if (hosts[i] == NULL)
        {
            logger(1, LOG_LEVEL, MY_SERVER_ID, "Error on malloc");
            exit(1);
        }
        fgets(hosts[i], 255, fp);
        char *newline_pos;
        if ((newline_pos = strchr(hosts[i], '\n')) != NULL)
            *newline_pos = '\0';
        logger(0, LOG_LEVEL, MY_SERVER_ID, "Host %s read in\n", hosts[i]);
        if ((strcmp(hosts[i], hostname)) == 0)
        {
            MY_SERVER_ID = i + 1;
            logger(0, LOG_LEVEL, MY_SERVER_ID, "Process id: %d\n", MY_SERVER_ID);
        }
    }

    return hosts;
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "USAGE:\n paxos -p port -h hostfile\n");
        exit(1);
    }

    if (strcmp(argv[1], "-p") == 0)
    {
        PORT = argv[2];
    }
    if (atoi(PORT) < 10000 || atoi(PORT) > 65535)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "Port number out of range 10000 - 65535\n");
        exit(1);
    }
    if (strcmp(argv[3], "-h") == 0)
    {
        PEERS = open_parse_hostfile(argv[4]);
    }

    logger(0, LOG_LEVEL, MY_SERVER_ID, "Command line parsed\n");

    // networking schtuff
    fd_set master;   // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax;       // maximum file descriptor num

    int listener; // listening socket descriptor
    // int newfd;                              // newly accept()ed socket descriptor
    struct sockaddr_storage their_addr; // client address
    socklen_t addrlen;

    int nbytes;

    int yes = 1; // for setsockopt() SO_REUSEADDR
    int i, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master); // clear master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // prevent "address already in use"
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "selectserver: failed to bind\n");
        exit(1);
    }
    freeaddrinfo(ai);

    if (listen(listener, 10) == -1)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "listen fail\n");
        exit(1);
    }

    // add listener to master set
    FD_SET(listener, &master);
    fdmax = listener;

    logger(0, LOG_LEVEL, MY_SERVER_ID, "Networking setup complete\n");

    initialize_globals();
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Globals initialized\n");

    struct timeval cur_time;    // for checking if timers have expired
    struct timeval select_timeout;

    for (;;)
    {

        // if Progress timer has expired
        if (PROGRESS_TIMER_SET)
        {
            logger(0, LOG_LEVEL, MY_SERVER_ID, "Checking progress timer\n");
            gettimeofday(&cur_time, NULL);
            if ((cur_time.tv_sec - PROGRESS_TIMER.tv_sec) > PROGRESS_TIMEOUT)
            {
                logger(0, LOG_LEVEL, MY_SERVER_ID, "Progress timer expired\n");
                shift_to_leader_election(LAST_ATTEMPTED + 1);
            }
        }

        // check is update timers have expired
        // ??? Is there somewhere else this could be happening?
        logger(0, LOG_LEVEL, MY_SERVER_ID, "Checking update timers\n");
        int j;
        for (j = 0; j < MAX_CLIENT_ID; j++)
        {    
            gettimeofday(&cur_time, NULL);
            if (((uint32_t)cur_time.tv_sec - UPDATE_TIMER[j]) > UPDATE_TIMEOUT)
            {
                logger(0, LOG_LEVEL, MY_SERVER_ID, "Update timer for %d expired\n", j);
                update_timer_expired(j);
            }
        }

        read_fds = master; // copy fd set
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 500000;

        if (select(fdmax + 1, &read_fds, NULL, NULL, &select_timeout) == -1)
        {
            logger(1, LOG_LEVEL, MY_SERVER_ID, "Failed on select\n");
            exit(1);
        }

        for (i = 0; i <= fdmax; i++)
        {
            // something to read on this socket
            if (FD_ISSET(i, &read_fds))
            {
                unsigned char *recvd_header = malloc(sizeof(Header));
                Header *header = malloc(sizeof(Header));

                if ((nbytes = recvfrom(listener, recvd_header, sizeof(Header), 0,
                                       (struct sockaddr *)&their_addr, &addrlen)) != sizeof(Header))
                {
                    logger(1, LOG_LEVEL, MY_SERVER_ID, "Receive length\n");
                    exit(1);
                }
                unpack_header(header, recvd_header);

                switch (header->msg_type)
                {
                // received client update
                // ??? I'm pretty sure this is the level of interaction where the third party
                // application ends up interacting with the paxos service
                case Client_Update_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received client update\n");

                    unsigned char *recvd_cupdate = malloc(header->size);
                    Client_Update *cupdate = malloc(sizeof(Client_Update));

                    if ((nbytes = recvfrom(listener, recvd_cupdate, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_client_update(cupdate, recvd_cupdate);

                    // ??? No conflict check for client updates?

                    client_update_handler(cupdate);
                    free(recvd_cupdate);

                    break;

                // received view change
                case View_Change_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received view change\n");

                    unsigned char *recvd_vc = malloc(header->size);
                    View_Change *v = malloc(sizeof(View_Change));

                    if ((nbytes = recvfrom(listener, recvd_vc, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_view_change(v, recvd_vc);

                    // conflict checks return true if there is a conflict
                    if (check_view_change(v))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received view change has conflicts\n");
                        break;
                    }

                    received_view_change(v);
                    free(recvd_vc);

                    break;

                // received vc proof
                case VC_Proof_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received vc proof\n");

                    unsigned char *recvd_proof = malloc(header->size);
                    VC_Proof *proof = malloc(sizeof(VC_Proof));

                    if ((nbytes = recvfrom(listener, recvd_proof, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_vc_proof(proof, recvd_proof);

                    //conflict checks
                    if (check_vc_proof(proof))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received vc proof has conflicts\n");
                        break;
                    }

                    received_vc_proof(proof);
                    free(recvd_proof);

                    break;

                // received prepare
                case Prepare_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received prepare\n");

                    unsigned char *recvd_prep = malloc(header->size);
                    Prepare *prep = malloc(sizeof(Prepare));

                    if ((nbytes = recvfrom(listener, recvd_prep, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_prepare(prep, recvd_prep);

                    if (check_prepare(prep))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received prepare has conflicts\n");
                        break;
                    }

                    received_prepare(prep);
                    free(recvd_prep);

                    break;

                // received prepare ok
                case Prepare_OK_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received prepare ok\n");

                    unsigned char *recvd_ok = malloc(header->size);
                    Prepare_OK *ok = malloc(sizeof(Prepare_OK));

                    if ((nbytes = recvfrom(listener, recvd_ok, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_prepare_ok(ok, recvd_ok);

                    if (check_prepare_ok(ok))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received prepare ok has conflicts\n");
                        break;
                    }

                    received_prepare_ok(ok);
                    free(recvd_ok);

                    break;

                // received proposal
                case Proposal_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received proposal\n");

                    unsigned char *recvd_prop = malloc(header->size);
                    Proposal *prop = malloc(sizeof(Proposal));

                    if ((nbytes = recvfrom(listener, recvd_prop, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_proposal(prop, recvd_prop);

                    if (check_proposal(prop))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received proposal has conflicts\n");
                        break;
                    }

                    received_proposal(prop);
                    free(recvd_prop);

                    break;

                // received accept
                case Accept_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received accept\n");

                    unsigned char *recvd_acc = malloc(header->size);
                    Accept *acc = malloc(sizeof(Accept));

                    if ((nbytes = recvfrom(listener, recvd_acc, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_accept(acc, recvd_acc);

                    if (check_accept(acc))
                    {
                        logger(0, LOG_LEVEL, MY_SERVER_ID,
                               "Received accept has conflicts\n");
                        break;
                    }

                    received_accept(acc);
                    free(recvd_acc);

                    break;

                // received globally ordered update
                case Globally_Ordered_Update_Type:
                    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received globally ordered update\n");

                    unsigned char *recvd_update = malloc(header->size);
                    Globally_Ordered_Update *update = malloc(sizeof(Globally_Ordered_Update));

                    if ((nbytes = recvfrom(listener, recvd_update, header->size, 0,
                                           (struct sockaddr *)&their_addr, &addrlen)) != header->size)
                    {
                        logger(1, LOG_LEVEL, MY_SERVER_ID, "Received %d bytes not %d\n",
                               nbytes, header->size);
                        exit(1);
                    }
                    unpack_global_ordered(update, recvd_update);

                    // ??? No conflict check for globally ordered update?

                    apply_globally_ordered_update(update);
                    free(recvd_update);

                    break;
                }

                free(recvd_header);
                free(header);
            }
        }
    }
}
