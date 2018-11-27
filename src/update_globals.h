#ifndef _UPDATEH_
#define _UPDATEH_

#include <stdbool.h>

#include "data_structures.h"
#include "messages.h"

extern int MY_SERVER_ID;
extern int STATE;
extern int LAST_INSTALLED;
extern Global_Slot **GLOBAL_HISTORY;
extern View_Change **VC;
extern Prepare *PREPARED;
extern int LAST_PROPOSED;
extern int LOCAL_ARU;
extern int LAST_ATTEMPTED;
extern node_t *UPDATE_QUEUE;
extern int UPDATE_QUEUE_SIZE;
extern int NUM_PEERS;
extern Prepare **PREPARE_OKS;
extern bool PROGRESS_TIMER_SET;

// Conflict checks to run on incoming messages.
// Messages for which a conflict exists are discarded.
// If the function returns true then there is a conflit.
bool check_view_change(View_Change *msg);
bool check_vc_proof(VC_Proof *msg);
bool check_prepare(Prepare *msg);
bool check_prepare_ok(Prepare_OK *msg);
bool check_proposal(Proposal *msg);
bool check_accept(Accept *msg);

// rules for updating the Global_history
//in the Paxos for Systems Builders documentation this is referred to as
// "applying" the message
void apply_view_change(View_Change *msg);
void apply_prepare(Prepare *msg);
void apply_prepare_ok(Prepare_OK *msg);
void apply_proposal(Proposal *msg);
void apply_accept(Accept *msg);
void apply_globally_ordered_update(Globally_Ordered_Update *msg);

void shift_to_reg_leader();
void shift_to_reg_non_leader();

#endif
