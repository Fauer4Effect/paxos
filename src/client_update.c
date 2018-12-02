#include <stdlib.h>
#include <time.h>

#include "client_update.h"
#include "messages.h"
#include "data_structures.h"
#include "global_ordering.h"
#include "serialize.h"
#include "multicast.h"

void client_update_handler(Client_Update *u)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "In client update handler\n");

    if (STATE == LEADER_ELECTION)
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "State: LEADER ELECTION\n");
        if (u->server_id == MY_SERVER_ID)
            return;
        if (enqueue_update(u))
            add_to_pending_updates(u);
    }
    if (STATE == REG_NONLEADER)
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "State: REG NON LEADER\n");
        if (u->server_id == MY_SERVER_ID)
        {
            add_to_pending_updates(u);

            // send to leader, u
            unsigned char *update_buf = malloc(sizeof(Client_Update));
            pack_client_update(u, update_buf);

            Header *head = malloc(sizeof(Header));
            head->msg_type = Client_Update_Type;
            head->size = sizeof(Client_Update);
            unsigned char *head_buf = malloc(sizeof(Header));
            pack_header(head, head_buf);

            int leader_id = LAST_INSTALLED % NUM_PEERS;

            send_to_single_host(head_buf, sizeof(Header), update_buf, head->size, leader_id);

            free(head_buf);
            free(head);
            free(update_buf);
        }
    }
    if (STATE == REG_LEADER)
    {
        logger(0, LOG_LEVEL, MY_SERVER_ID, "State: REG LEADER\n");
        if (enqueue_update(u))
        {
            if (u->server_id == MY_SERVER_ID)
                add_to_pending_updates(u);
            send_proposal();
        }
    }
}

void update_timer_expired(int client_id)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Update time expired\n");
    // restart update timer (client id)
    UPDATE_TIMER[client_id] = (unsigned)time(NULL);
    if (STATE == REG_NONLEADER)
    {
        // Send to leader pending_updates[client_id]
        Client_Update *pending = PENDING_UPDATES[client_id];
        unsigned char *pending_buf = malloc(sizeof(Client_Update));
        pack_client_update(pending, pending_buf);

        Header *head = malloc(sizeof(Header));
        head->msg_type = Client_Update_Type;
        head->size = sizeof(Client_Update);
        unsigned char *head_buf = malloc(sizeof(Header));
        pack_header(head, head_buf);

        int leader_id = LAST_INSTALLED % NUM_PEERS;

        send_to_single_host(head_buf, sizeof(Header), pending_buf, head->size, leader_id);

        free(head_buf);
        free(head);
        free(pending_buf);
    }
}

bool enqueue_update(Client_Update *u)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Checking enqueue update\n");
    if (u->timestamp <= LAST_EXECUTED[u->client_id])
        return false;
    if (u->timestamp <= LAST_ENQUEUED[u->client_id])
        return false;

    // add u to the update queue
    append_to_list(u, UPDATE_QUEUE, Client_Update_Type);

    LAST_ENQUEUED[u->client_id] = u->timestamp;
    return true;
}

void add_to_pending_updates(Client_Update *u)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Adding to pending updates\n");
    PENDING_UPDATES[u->client_id] = u;
    // Set update timer(u->client id)
    UPDATE_TIMER[u->client_id] = (unsigned)time(NULL);

    // XXX SYNC to disk
}

// FIXME need to know what a bound update is
void enqueue_unbound_pending_updates()
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Enqueue unbound pending updates\n");
    Client_Update *u;
    int i;
    for (i = 0; i < MAX_CLIENT_ID; i++)
    {
        u = PENDING_UPDATES[i];
        // XXX If U is not bound and U is not in Update_Queue
        //if()
        //  enqueue_update(u);
    }
}

// FIXME how to tell when something is bound
void remove_bound_updates_from_queue()
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Remove bound updates\n");
    Client_Update *u;
    int i;
    for (i = 0; i < list_length(UPDATE_QUEUE); i++)
    {
        u = get_index(UPDATE_QUEUE, i);
        // XXX if U is bound or u->timestamp <= LAST_EXECUTED[u->client_id] or
        // (if u->timestamp <= LAST_ENQUEUED[u->client_id] and u->server_id != MY_SERVER_ID)
        //  remove u from update_queue
        //  if (u->timestanp > Last_Enqueued[u->client_id])
        //      last_enqueue[u->client_id] = u->timestamp;
    }
}
