#ifndef _LEADERH_
#define _LEADERH_

#include "messages.h"
#include "logging.h"

#define LOG_LEVEL DEBUG

extern int NUM_PEERS;
extern View_Change *VC[];
extern Prepare *PREPARED;
extern Prepare *PREPARE_OKS[];
extern int MAX_CLIENT_ID;
extern int LAST_ATTEMPTED;
extern int MY_SERVER_ID;
extern uint32_t *LAST_ENQUEUED[];
extern bool PROGRESS_TIMER_SET;
extern uint32_t PROGRESS_TIMEOUT;
extern struct timeval PROGRESS_TIMER;
extern int LAST_INSTALLED;

bool preinstall_ready(int view);
void shift_to_leader_election(int view);
void received_view_change(View_Change *v);
void received_vc_proof(VC_Proof *v);

#endif