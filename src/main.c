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

#include "client_update.h"
#include "data_structures.h"
#include "global_ordering.h"
#include "leader_election.h"
#include "logging.h"
#include "messages.h"
#include "multicast.h"
#include "prepare.h"
#include "serialize.h"
#include "update_globals.h"

#define LOG_LEVEL DEBUG                 // set log level to debug for logging
int MAX_CLIENT_ID  = 32;                // size of arrays that are indexed by client id
                                        // updated by increase_array_size()

// server state variables   
int MY_SERVER_ID;                       // unique identifier for this server
int NUM_PEERS;                          // how many peers in the protocol
char *PORT;                             // what port is it listening on
char *PEERS[];                          // who all the list of peers
int STATE;                              // one of {LEADER_ELECTION, REG_LEADER, REG_NONLEADER}
                                        // defined in data_structures.h

// view state variables
int LAST_ATTEMPTED;                     // last view this server attempted to install
int LAST_INSTALLED;                     // last  view this server installed
View_Change *VC[];                      // array of View_Change messages, indexed by server_id
                                        // size of NUM_PEERS

// prepare phase variables
Prepare *PREPARED;                      // the Prepare message from last preinstalled view, if any
Prepare *PREPARE_OKS[];                 // array of Prepare_OK messages received, indexed by serv id
                                        // size of NUM_PEERS

// global ordering variables
int LOCAL_ARU;                          // local aru value of this server
int LAST_PROPOSED;                      // last sequence number proposed by the leader
Global_Slot *GLOBAL_HISTORY[];          // array of global_slots indexed by sequence number
                                        // size is dynamic by increase_array_size()
                                        // defined in data_structures.h

// timer variables
struct timeval PROGRESS_TIMER;          // timeout on making global progress
bool PROGRESS_TIMER_SET;                // keep track of whether the progress time was set
uint32_t PROGRESS_TIMEOUT;              // how long to wait until PROGRESS_TIMER is timed out
uint32_t *UPDATE_TIMER[];               // array of timeouts, indexed by client id
                                        // dynamic size by increase_array_size()

// client handling variables
node_t *UPDATE_QUEUE;                   // queue of Client_Update messages
uint32_t *LAST_EXECUTED[];              // array of timestamps, indexed by client_id
uint32_t *LAST_ENQUEUED[];              // array of timestamps, indexed by client_id
                                        // stores tv_sec field in struct timeval
                                        // dynamic size
Client_Update *PENDING_UPDATES[];       // array of Client_Update messages, indexed by client_id
                                        // dynamic size

// TODO when initializing node_t objects (really all of the structs)
// need to explicitly set the pointers to null and make sure that all the code
// expects those to be null not zero

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char **open_parse_hostfile(char *hostfile)
{
    char hostname[64];
    int cur_hostname = gethostname(hostname, 63);
    logger(0, LOG_LEVEL, PROCESS_ID, "Got hostname: %s\n", hostname);
    if (cur_hostname == -1)
    {
        logger(1, LOG_LEVEL, PROCESS_ID, "Could not get hostname\n");
        exit(1);
    }

    FILE *fp = fopen(hostfile, "r");
    if (fp == NULL)
    {
        logger(1, LOG_LEVEL, PROCESS_ID, "Could not open hostfile\n");
        exit(1);
    }
    logger(0, LOG_LEVEL, PROCESS_ID, "Opened hostfile\n");

    // get number of lines in hostfile
    int numLines = 0;
    char chr;
    chr = getc(fp);
    while (chr != EOF) 
    {
        if (chr == '\n')
            numLines ++;
        chr = getc(fp);
    }
    logger(0, LOG_LEVEL, PROCESS_ID, "Hostfile is %d lines\n", numLines);
    fseek(fp, 0, SEEK_SET);     // reset to beginning

    // malloc 2-D array of hostnames
    char **hosts = malloc(numLines * sizeof(char *));
    // keep track of size of host file
    NUM_HOSTS = numLines;
    if (hosts == NULL)
    {
        logger(1, LOG_LEVEL, PROCESS_ID, "Could not malloc hosts array\n");
        exit(1);
    }

    // read in all the host names
    int i = 0;
    for(i = 0; i < NUM_HOSTS; i++)
    {
        hosts[i] = (char *)malloc(255);
        if (hosts[i] == NULL)
        {
            logger(1, LOG_LEVEL, PROCESS_ID, "Error on malloc");
            exit(1);
        }
        fgets(hosts[i], 255, fp);
        char *newline_pos;
        if ((newline_pos=strchr(hosts[i], '\n')) != NULL)
            *newline_pos = '\0';
        logger(0, LOG_LEVEL, PROCESS_ID, "Host %s read in\n", hosts[i]);
        if ((strcmp(hosts[i], hostname)) == 0)
        {
            PROCESS_ID = i+1;
            logger(0, LOG_LEVEL, PROCESS_ID, "Process id: %d\n", PROCESS_ID);
        }
    }

    return hosts;
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        logger(1, LOG_LEVEL, PROCESS_ID, "USAGE:\n paxos -p port -h hostfile\n");
        exit(1);
    }

    if (strcmp(argv[1], "-p") == 0)
    {
        PORT = argv[2];
    }
    if (atoi(PORT) < 10000 || atoi(PORT) > 65535)
    {
        logger(1, LOG_LEVEL, PROCESS_ID, "Port number out of range 10000 - 65535\n");
        exit(1);
    }
    if (strcmp(argv[3], "-h") == 0)
    {
        PEERS = open_parse_hostfile(argv[4]);
    }

    logger(0, LOG_LEVEl, PROCESS_ID, "Command line parsed\n");
}