

#include "messages.h"

#define LEADER_ELECTION 1;
#define REG_LEADER 2;
#define REG_NONLEADER 3;

extern int MAX_CLIENT_ID;

typedef struct {
    Proposal *proposal;         // latest Proposal accepted for this sequence number, if any
    Accept *accepts[];           // array of corresponding Accept messages, indexed by server_id
    Globally_Ordered_Update *global_ordered_update;
                                // ordered update for this sequence number, if any
} Global_Slot;

typedef struct node {
    void *data;
    struct node *next;
} node_t;

int list_length(node_t *list);
int append_to_list(void *new_data, node_t *list);
int clear_list(node_t *list);
void *pop_from_queue(node_t *q);
uint32_t *increase_array_size(uint32_t *old_array);