#ifndef _GLOBALH_
#define _GLOBALH_

#include <stdbool.h>

#include "messages.h"
#include "data_structures.h"
#include "update_globals.h"

extern int LAST_PROPOSED;
extern int MY_SERVER_ID;
extern int LAST_INSTALLED;
extern Global_Slot **GLOBAL_HISTORY;
extern Client_Update **PENDING_UPDATES;
extern uint32_t *LAST_EXECUTED;
extern int STATE;
extern struct timeval PROGRESS_TIMER;
extern node_t *UPDATE_QUEUE;
extern int NUM_PEERS;
extern int LOCAL_ARU;
extern int LOG_LEVEL;
extern bool PROGRESS_TIMER_SET;
extern uint32_t *UPDATE_TIMER;
extern int LEADER_ELECTION;
extern int REG_LEADER;
extern bool TEST_UPDATE_READY;

void received_proposal(Proposal *p);
void received_accept(Accept *acc);
void executed_client_update(Client_Update *u);
void send_proposal();
bool globally_ordered_ready(int seq);
void advance_aru();

#endif
