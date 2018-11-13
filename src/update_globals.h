#include <stdbool.h>

#include "data_structures.h"

extern int MY_SERVER_ID;
extern int STATE;
extern int LAST_INSTALLED;
extern Global_Slot *GLOBAL_HISTORY[];
extern View_Change *VC[];
extern Prepare *PREPARED;

// Conflict checks to run on incoming messages. Messages for which
// a conflict exists are discarded
bool check_view_change(View_Change *msg);
bool check_vc_proof(VC_Proof *msg);
bool check_prepare(Prepare *msg);
bool check_prepare_ok(Prepare_OK *msg);
bool check_proposal(Proposal *msg);
bool check_accept(Accept *msg);

// rules for updating the Global_history
void update_view_change(View_Change *msg);
void update_prepare(Prepare *msg);
void update_prepare_ok(Prepare_OK *msg);
void update_proposal(Proposal *msg);
void update_accept(Accept *msg);
void update_globally_ordered_update(Globally_Ordered_Update *msg);
