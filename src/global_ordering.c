#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>

#include "messages.h"
#include "global_ordering.h"
#include "serialize.h"
#include "multicast.h"
#include "data_structures.h"

void received_proposal(Proposal *p)
{
    // XXX apply proposal
    LAST_PROPOSED = p->seq;
    GLOBAL_HISTORY[p->seq]->proposal = p;
    Accept *acc = malloc(sizeof(Accept));
    acc->server_id = MY_SERVER_ID;
    acc->seq = LAST_PROPOSED;
    acc->view = LAST_INSTALLED;
    unsigned char *acc_buf = malloc(sizeof(Accept));
    pack_accept(acc, acc_buf);

    // XXX sync to disk

    Header *head = malloc(sizeof(Header));
    head->msg_type = Accept_Type;
    head->size = sizeof(acc_buf);
    unsigned char *head_buf = malloc(sizeof(Header));
    pack_header(head, head_buf);

    multicast(head_buf, sizeof(head_buf), acc_buf, sizeof(acc_buf));
    free(head_buf);
    free(head);
    free(acc_buf);
    free(acc);
}

// FIXME this probably broken
void received_accept(Accept *acc)
{
    // XXX apply accept
    LAST_PROPOSED = acc->seq;
    GLOBAL_HISTORY[acc->seq]->accepts[acc->server_id] = acc;
    if (globally_ordered_ready(acc->seq))
    {
        Globally_Ordered_Update *global = malloc(sizeof(Globally_Ordered_Update));
        global->server_id = MY_SERVER_ID;
        global->seq = acc->seq;
        // XXX apply globally ordered
        GLOBAL_HISTORY[acc->seq]->global_ordered_update = global;
        advance_aru();
    }
}

void executed_client_update(Client_Update *u)
{
    advance_aru();
    if (u->server_id == MY_SERVER_ID)
    {
        // XXX Reply to client
        if (PENDING_UPDATES[u->client_id] != 0)
        {
            // cancel update timer(client_id)
            // ??? should update timer be an array of timers
            PENDING_UPDATES[u->client_id] = 0;
        }
    }
    LAST_EXECUTED[u->client_id] = (unsigned)time(NULL);
    if (STATE != LEADER_ELECTION)
    {
        // XXX restart progress timer
        gettimeofday(&PROGRESS_TIMER, NULL);
    }
    if (STATE == REG_LEADER)
    {
        send_proposal();
    }
}

void send_proposal()
{
    int seq = LAST_PROPOSED+1;
    if (GLOBAL_HISTORY[seq]->global_ordered_update != NULL)
    {
        LAST_PROPOSED++;
        send_proposal();
    }

    if (GLOBAL_HISTORY[seq]->proposal != NULL)
    {
        Client_Update *u = GLOBAL_HISTORY[seq]->proposal->update;
    } else if (UPDATE_QUEUE[0] == NULL)
    {
        return;
    } else
    {
        Client_Update *u = UPDATE_QUEUE[0];
        UPDATE_QUEUE += 4;
    }
    
    Proposal *p = malloc(sizeof(Proposal));
    p->server_id = MY_SERVER_ID;
    p->view = LAST_INSTALLED;
    p->seq = seq;
    p->update = u;
    unsigned char *prop_buf = malloc(sizeof(Proposal));
    pack_proposal(p, prop_buf);
    // XXX SYNC TO DISK

    Header *head = malloc(sizeof(Header));
    head->msg_type = Proposal_Type;
    head->size = sizeof(prop_buf);
    unsigned char *head_buf = malloc(sizeof(Header));
    pack_header(head, head_buf);
    
    multicast(head_buf, sizeof(head_buf), prop_buf, sizeof(prop_buf));
    free(head_buf);
    free(head);
    free(prop_buf);
    free(p);
}

bool globally_ordered_ready(int seq)
{
    Proposal *p = GLOBAL_HISTORY[seq]->proposal;
    if (p == NULL)
        return false;
    Accept *accepts[] = GLOBAL_HISTORY[seq]->accepts;
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (accepts[i] != 0)
            count++;
    }
    if (count >= (NUM_PEERS/2))
        return true;
    return false;
}

void advance_aru()
{
    int i = LOCAL_ARU+1;
    while (1)
    {
        if (GLOBAL_HISTORY[i]->global_ordered_update != 0)
        {
            LOCAL_ARU++;
            i++;
        } else
        {
            return;
        }
    }
}