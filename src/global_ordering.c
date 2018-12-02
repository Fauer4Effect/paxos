#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "messages.h"
#include "global_ordering.h"
#include "serialize.h"
#include "multicast.h"
#include "data_structures.h"
#include "logging.h"

void received_proposal(Proposal *p)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received proposal\n");
    // apply proposal
    apply_proposal(p);

    Accept *acc = malloc(sizeof(Accept));
    acc->server_id = MY_SERVER_ID;
    acc->view = p->view;
    acc->seq = p->seq;
    unsigned char *acc_buf = malloc(sizeof(Accept));
    pack_accept(acc, acc_buf);

    // XXX sync to disk

    Header *head = malloc(sizeof(Header));
    head->msg_type = Accept_Type;
    head->size = sizeof(Accept);
    unsigned char *head_buf = malloc(sizeof(Header));
    pack_header(head, head_buf);

    multicast(head_buf, sizeof(Header), acc_buf, head->size);
    free(head_buf);
    free(head);
    free(acc_buf);
    free(acc);
}

void received_accept(Accept *acc)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received accept\n");
    // apply accept
    apply_accept(acc);

    if (globally_ordered_ready(acc->seq))
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "Globally ordered ready\n");
        Globally_Ordered_Update *global = malloc(sizeof(Globally_Ordered_Update));
        global->server_id = MY_SERVER_ID;
        global->seq = acc->seq;
        Client_Update *update = GLOBAL_HISTORY[acc->seq]->proposal->update;

        // apply globally ordered
        apply_globally_ordered_update(global);
        // ??? Does this mean that when we have applied the update that we have 'executed'
        // that update?
        executed_client_update(update);

        advance_aru();
    }
}

void executed_client_update(Client_Update *u)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Executed client update\n");
    advance_aru();
    if (u->server_id == MY_SERVER_ID)
    {
        // XXX Reply to client
        // logger(1, LOG_LEVEL, MY_SERVER_ID, "Executed client update %d\n", u->client_id);
        printf("--------------------- EXECUTED CLIENT UPDATE %d -----------------\n", u->client_id);

        if (PENDING_UPDATES[u->client_id] != 0)
        {
            // cancel update timer(client_id)
            // we can just used -1 to designate it as some impossible time when we have to
            // then go and check for progress
            UPDATE_TIMER[u->client_id] = -1;

            PENDING_UPDATES[u->client_id] = 0;
        }
    }
    // time(NULL) returns time_t object, struct timeval uses time_t to
    // represent tv_sec so just use this in the timer to store seconds
    LAST_EXECUTED[u->client_id] = (unsigned)time(NULL);

    if (STATE != LEADER_ELECTION)
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "Setting progress timer\n");
        // restart progress timer
        gettimeofday(&PROGRESS_TIMER, NULL);
        PROGRESS_TIMER_SET = true;
    }
    if (STATE == REG_LEADER)
    {
        send_proposal();
    }
}

void send_proposal()
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Sending proposal\n");
    int seq = LAST_PROPOSED + 1;
    Client_Update *u;
    if (GLOBAL_HISTORY[seq]->global_ordered_update != 0)
    {
        LAST_PROPOSED++;
        send_proposal();
    }

    if (GLOBAL_HISTORY[seq]->proposal != 0)
    {
        u = GLOBAL_HISTORY[seq]->proposal->update;
    }
    else if (list_length(UPDATE_QUEUE) == 0)
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "No updates to propose\n");
        // XXX For testing purposes we will send the client update now
        if (MY_SERVER_ID == 1)
            TEST_UPDATE_READY = true;
        return;
    }
    else
    {
        u = pop_from_queue(UPDATE_QUEUE);
    }

    Proposal *p = malloc(sizeof(Proposal));
    p->server_id = MY_SERVER_ID;
    p->view = LAST_INSTALLED; // XXX check that this is the right one
    p->seq = seq;
    p->update = u;

    apply_proposal(p);
    LAST_PROPOSED = seq;

    // XXX SYNC TO DISK

    // size of proposal is uint32_t * 3 + storage for client update
    // client update is uin32_t * 4
    unsigned char *prop_buf = malloc(sizeof(uint32_t) * 7);
    pack_proposal(p, prop_buf);

    Header *head = malloc(sizeof(Header));
    head->msg_type = Proposal_Type;
    // server_id + view + seq + Client_Update (4*uint32_T)
    head->size = (sizeof(uint32_t) * 7);
    unsigned char *head_buf = malloc(sizeof(Header));
    pack_header(head, head_buf);

    multicast(head_buf, sizeof(Header), prop_buf, head->size);
    free(head_buf);
    free(head);
    free(prop_buf);
    free(p);
}

bool globally_ordered_ready(int seq)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Check if globally ordered ready\n");
    Proposal *p = GLOBAL_HISTORY[seq]->proposal;
    if (p == NULL)
        return false;
    Accept **accepts = GLOBAL_HISTORY[seq]->accepts;
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (accepts[i] != 0)
            count++;
    }
    if (count >= (NUM_PEERS / 2))
        return true;
    return false;
}

void advance_aru()
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Advance aru\n");
    int i = LOCAL_ARU + 1;
    while (1)
    {
        if (GLOBAL_HISTORY[i]->global_ordered_update != NULL)
        {
            LOCAL_ARU++;
            i++;
        }
        else
            return;
    }
}
