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
#include "update_globals.h"
#include "serialize.h"
#include "multicast.h"
#include "logging.h"

#define LOG_LEVEL DEBUG                 // set log level to debug for logging
#define LAST_ENQUEUED_SIZE 32           // size of LAST_ENQUEUED[]

// server state variables   
int MY_SERVER_ID;                       // unique identifier for this server
int NUM_PEERS;                          // how many peers in the protocol
char *PORT;                             // what port is it listening on
char *PEERS[];                           // who all the list of peers
int STATE;                              // one of {LEADER_ELECTION, REG_LEADER, REG_NONLEADER}
                                        // defined in data_structures.h

// view state variables
int LAST_ATTEMPTED;                     // last view this server attempted to install
int LAST_INSTALLED;                     // last  view this server installed
View_Change *VC[];                      // array of View_Change messages, indexed by server_id

// prepare phase variables
Prepare *PREPARED;                      // the Prepare message from last preinstalled view, if any
Prepare *PREPARE_OKS[];                 // array of Prepare_OK messages received, indexed by serv id

// global ordering variables
int LOCAL_ARU;                          // local aru value of this server
int LAST_PROPOSED;                      // last sequence number proposed by the leader
Global_Slot *GLOBAL_HISTORY[];          // array of global_slots indexed by sequence number
                                        // defined in data_structures.h

// timer variables
struct timeval PROGRESS_TIMER;          // timeout on making global progress
bool PROGRESS_TIMER_SET;                // keep track of whether the progress time was set
uint32_t PROGRESS_TIMEOUT;              // how long to wait until PROGRESS_TIMER is timed out
struct timeval UPDATE_TIMER;            // timeout on globally ordering a specific update

// client handling variables
Client_Update *UPDATE_QUEUE[];          // queue of Client_Update messages
uint32_t *LAST_EXECUTED[];              // array of timestamps, indexed by client_id
uint32_t *LAST_ENQUEUED[];              // array of timestamps, indexed by client_id
Client_Update *PENDING_UPDATES[];       // array of Client_Update messages, indexed by client_id

bool preinstall_ready(int view)
{
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (VC[i]->attempted == view)
            count++;
    }
    if (count >= ((NUM_PEERS/2)+1))
        return true;
    return false;
}

void shift_to_leader_election(int view)
{
    // ??? VC[] indexed by server id so should only be as big as num peers?
    int i;
    for (i = 0; i < NUM_PEERS; i++)
    {
        free(VC[i]);
        VC[i] = 0;
    }
    free(PREPARED);

    // ??? size of prepare_oks[] should just be size of num peers right?
    for (i = 0; i < NUM_PEERS; i++)
    {
        free(PREPARE_OKS[i]);
        PREPARE_OKS[i] = 0;
    }

    for (i = 0; i < LAST_ENQUEUED_SIZE; i++)
    {
        free(LAST_ENQUEUED[i]);
        LAST_ENQUEUED[i] = 0;
    }

    LAST_ATTEMPTED = view;
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Cleared data structures\n");

    View_Change *vc = malloc(sizeof(View_Change));
    vc->server_id = MY_SERVER_ID;
    vc->attempted = LAST_ATTEMPTED;
    unsigned char *vc_buf = malloc(sizeof(View_Change));
    pack_view_change(vc, vc_buf);

    Header *header = malloc(sizeof(Header));
    header->msg_type = View_Change_Type;
    header->size = sizeof(vc_buf);
    unsigned char *header_buf = malloc(sizeof(Header));
    pack_header(header, header_buf);

    multicast(header_buf, sizeof(header_buf), vc_buf, sizeof(vc_buf));
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Multicast View Change %d\n", vc->attempted);

    free(header_buf);
    free(header);
    free(vc_buf);

    // ??? Apply vc to data structures? Just update VC[]?
    VC[MY_SERVER_ID] = vc;
}

void received_view_change(View_Change *v)
{
    if ((v->attempted > LAST_ATTEMPTED) && PROGRESS_TIMER_SET)
    {
        shift_to_leader_election(v->attempted);
        // ??? Apply V? does this just mean update VC[]
        VC[v->server_id] = v;
    }
    if (v->attempted = LAST_ATTEMPTED)
    {
        VC[v->server_id] = v;
        if (preinstall_ready(v->attempted))
        {
            PROGRESS_TIMEOUT *= 2;
            // set the progress timer
            gettimeofday(&PROGRESS_TIMER, NULL);
            // if leader of LAST_ATTEMPTED
            if ((LAST_ATTEMPTED % NUM_PEERS) == MY_SERVER_ID)
            {
                shift_to_prepare_phase();
            }
        }
    }
}