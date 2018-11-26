#ifndef _DATAH_
#define _DATAH_

#include "messages.h"
#include "logging.h"

#define LEADER_ELECTION 1;
#define REG_LEADER 2;
#define REG_NONLEADER 3;

extern int MAX_CLIENT_ID;
extern int LOG_LEVEL;
extern int MY_SERVER_ID;

typedef struct
{
    Proposal *proposal; // latest Proposal accepted for this sequence number, if any
    Accept *accepts[];  // array of corresponding Accept messages, indexed by server_id
                        // size of NUM_PEERS
    Globally_Ordered_Update *global_ordered_update;
    // ordered update for this sequence number, if any
} Global_Slot;

typedef struct node_t
{
    void *data;        // pointer to arbitrary data
    struct node_t *next; // pointer to next node
    uin32_t data_type; // normally you would only want to have a single data type in the
                       // list but the data_list in the prepare_ok message has two kinds
                       // so we can support this as wellS
} node_t;

int list_length(node_t *list);
int append_to_list(void *new_data, node_t *list, int data_type);
int clear_list(node_t *list);
void *pop_from_queue(node_t *q);
node_t *get_index(node_t *list, int index)
    uint32_t *increase_array_size(uint32_t *old_array);


#endif