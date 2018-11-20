

#include "client_update.h"
#include "messages.h"
#include "data_structures.h"
#include "global_ordering.h"

void client_update_handler(Client_Update *u)
{
    if (STATE == LEADER_ELECTION)
    {
        if (u->server_id == MY_SERVER_ID)
            return;
        if (enqueue_update(u))
            add_to_pending_updates(u);
    }
    if (STATE == REG_NONLEADER)
    {
        if (u->server_id == MY_SERVER_ID)
        {
            add_to_pending_updates(u);
            // XXX send to leader, u

        }
    }
    if (STATE == REG_LEADER)
    {
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
    // XXX restart update timer (client id)
    if (STATE == REG_NONLEADER)
        // XXX Send to leader pending_updates[client_id]
}

bool enqueue_update(Client_Update *u)
{
    if (u->timestamp <= LAST_EXECUTED[u->client_id])
        return false;
    if (u->timestamp <= LAST_ENQUEUED[u->client_id])
        return false;
    // XXX add u to the update queue
    LAST_ENQUEUED[u->client_id] = u->timestamp;
    return true;
}

void add_to_pending_updates(Client_Update *u)
{
    PENDING_UPDATES[u->client_id] = u;
    // XXX Set update timer(u->client id)
    // XXX SYNC to disk
}

// TODO Code this
void enqueue_unbound_pending_updates()
{

}

// TODO Code this
void remove_bound_updates_from_queue()
{

}