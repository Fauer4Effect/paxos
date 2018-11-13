#include <stdbool.h>

#include "data_structures.h"

extern int MY_SERVER_ID;
extern int STATE;
extern int LAST_INSTALLED;
extern Global_Slot *GLOBAL_HISTORY[];

// Conflict checks to run on incoming messages. Messages for which
// a conflict exists are discarded
bool check_view_change(View_Change *msg);
bool check_vc_proof(VC_Proof *msg);
bool check_prepare(Prepare *msg);
bool check_prepare_ok(Prepare_OK *msg);
bool check_proposal(Proposal *msg);
bool check_accept(Accept *msg);