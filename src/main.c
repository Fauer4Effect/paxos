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

