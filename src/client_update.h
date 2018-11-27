#ifndef _CLIENTH_
#define _CLIENTH_

#include <stdbool.h>

#include "messages.h"

extern int STATE;
extern int MY_SERVER_ID;
extern uint32_t *LAST_ENQUEUED;
extern uint32_t *UPDATE_TIMER;

void client_update_handler(Client_Update *u);
void update_timer_expired(int client_id);
bool enqueue_update(Client_Update *u);
void add_to_pending_updates(Client_Update *u);
void enqueue_unbound_pending_updates();
void remove_bound_updates_from_queue();

#endif
