#include <stdbool.h>
#include <stdio.h>

#include "messages.h"
#include "conflict_checks.h"
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