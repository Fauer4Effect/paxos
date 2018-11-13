

#include "messages.h"

#define LEADER_ELECTION 1;
#define REG_LEADER 2;
#define REG_NONLEADER 3;

typedef struct {
    Proposal *proposal;         // latest Proposal accepted for this sequence number, if any
    Accept **accepts;           // array of corresponding Accept messages, indexed by server_id
    Globally_Ordered_Update *global_ordered_update;
                                // ordered update for this sequence number, if any
} Global_Slot;