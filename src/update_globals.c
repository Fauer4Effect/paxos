#include <stdbool.h>
#include <stdio.h>

#include "messages.h"
#include "data_structures.h"

bool check_view_change(View_Change *msg)
{
    if (msg->server_id == MY_SERVER_ID)
        return true;
    if (STATE != LEADER_ELECTION)
        return true;
    if (msg->attempted <= LAST_INSTALLED)
        return true;
    return false;
}

bool check_vc_proof(VC_Proof *msg)
{
    if (msg->server_id == MY_SERVER_ID)
        return true;
    if (STATE != LEADER_ELECTION)
        return true;
    return false;
}

bool check_prepare(Prepare *msg)
{
    if (msg->server_id == MY_SERVER_ID)
        return true;
    if (msg->view != LAST_ATTEMPTED)
        return true;
    return false;
}

bool check_prepare_ok(Prepare_OK *msg)
{
    if (STATE != LEADER_ELECTION)
        return true;
    if (msg->view != LAST_ATTEMPTED)
        return true;
    return false;
}

bool check_proposal(Proposal *msg)
{
    if (msg->server_id == MY_SERVER_ID)
        return true;
    if (STATE != REG_NONLEADER)
        return true;
    if (msg->view != LAST_INSTALLED)
        return true;
    return false;
}

bool check_accept(Accept *msg)
{
    if (msg->server_id == MY_SERVER_ID)
        return true;
    if (msg->view != LAST_INSTALLED)
        return true;
    if (GLOBAL_HISTORY[msg->seq]->proposal == NULL)
        return true;
    return false;
}

void update_view_change(View_Change *msg)
{
    if (VC[msg->server_id] != NULL)
        return;
    VC[msg->server_id] = msg;
}

void update_prepare(Prepare *msg)
{
    PREPARED = msg;
}

void update_prepare_ok(Prepare_OK *msg)
{
    if (PREPARE_OKS[msg->server_id] != NULL)
        return;
    PREPARE_OKS[msg->server_id] = msg;
    // XXX right now we won't be doing reconciliation, so we know that the
    // only thing in the data_list should be proposals
    Proposal *e;
    int i;
    for (i = 0; i < msg->data_list_size; i++)
    {
        e = msg->data_list[i];
        update_proposal(e);
    }
}

void update_proposal(Proposal *msg)
{
    if (GLOBAL_HISTORY[msg->seq]->global_ordered_update != NULL)
        return;
    Proposal *p_prime = GLOBAL_HISTORY[msg->seq];
    if (p_prime != NULL)
    {
        if (msg->view > p_prime->view)
        {
            GLOBAL_HISTORY[msg->seq]->proposal = msg;
            int i;
            for (int i = 0; i < NUM_PEERS; i++)
            {
                GLOBAL_HISTORY[msg->seq]->accepts[i] = NULL;
            }
        }
    } else {
        GLOBAL_HISTORY[msg->seq]->proposal = msg;
    }
}

void update_accept(Accept *msg)
{
    if (GLOBAL_HISTORY[msg->seq]->global_ordered_update != NULL)
        return;
    int i;
    int num_accepts = 0;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (GLOBAL_HISTORY[msg->seq]->accepts[i] != NULL)
            num_accepts++;
    }
    if (num_accepts > (NUM_PEERS / 2))
        return;
    if (GLOBAL_HISTORY[msg->seq]->accepts[msg->server_id] != NULL)
        return;
    GLOBAL_HISTORY[msg->seq]->accepts[msg->server_id] = msg;
}

void update_globally_ordered_update(Globally_Ordered_Update *msg)
{
    if (GLOBAL_HISTORY[msg->seq]->global_ordered_update == NULL)
        GLOBAL_HISTORY[msg->seq]->global_ordered_update = msg;
}
