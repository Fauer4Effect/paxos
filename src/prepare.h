#ifndef _PREPAREH_
#define _PREPAREH_


#include "messages.h"
#include "update_globals.h"

extern int LAST_INSTALLED;
extern int LAST_ATTEMPTED;
extern int MY_SERVER_ID;
extern int LOCAL_ARU;
extern Prepare *PREPARED;
extern Global_Slot **GLOBAL_HISTORY;
extern Prepare **PREPARE_OKS;
extern int LAST_ENQUEUED_SIZE;
extern uint32_t *LAST_ENQUEUED;
extern int STATE;
extern int NUM_PEERS;
extern int MAX_CLIENT_ID;
extern int LEADER_ELECTION;

node_t *construct_datalist(int aru);
bool view_prepared_ready(int view);
void shift_to_prepare_phase();
void received_prepare(Prepare *prepare);
void received_prepare_ok(Prepare_OK *prepare_ok);

#endif
