#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#include "leader_election.h"
#include "update_globals.h"
#include "serialize.h"
#include "multicast.h"

bool preinstall_ready(int view)
{
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (VC[i]->attempted == view)
            count++;
    }
    if (count >= ((NUM_PEERS / 2) + 1))
        return true;
    return false;
}

void shift_to_leader_election(int view)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Shift to leader election\n");

    int i;
    for (i = 0; i < NUM_PEERS; i++)
    {
        free(VC[i]);
        VC[i] = 0;
    }
    free(PREPARED);
    PREPARED = 0;

    // ??? size of prepare_oks[] should just be size of num peers right?
    for (i = 0; i < NUM_PEERS; i++)
    {
        free(PREPARE_OKS[i]);
        PREPARE_OKS[i] = 0;
    }

    for (i = 0; i < MAX_CLIENT_ID; i++)
    {
        LAST_ENQUEUED[i] = 0;
    }

    LAST_ATTEMPTED = view;
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Cleared data structures\n");

    View_Change *vc = malloc(sizeof(View_Change));
    vc->server_id = MY_SERVER_ID;
    vc->attempted = LAST_ATTEMPTED;
    unsigned char *vc_buf = malloc(sizeof(View_Change));
    pack_view_change(vc, vc_buf);

    Header *header = malloc(sizeof(Header));
    header->msg_type = View_Change_Type;
    header->size = sizeof(View_Change);
    unsigned char *header_buf = malloc(sizeof(Header));
    pack_header(header, header_buf);

    multicast(header_buf, sizeof(Header), vc_buf, header->size);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Multicast View Change %d\n", vc->attempted);

    free(header_buf);
    free(header);
    free(vc_buf);

    // Apply vc to data structures? Just update VC[]?
    apply_view_change(vc);
}

void received_view_change(View_Change *v)
{
    if ((v->attempted > LAST_ATTEMPTED) && !(PROGRESS_TIMER_SET))
    {
        shift_to_leader_election(v->attempted);
        apply_view_change(v);
    }
    if (v->attempted == LAST_ATTEMPTED)
    {
        apply_view_change(v);
        if (preinstall_ready(v->attempted))
        {
            PROGRESS_TIMEOUT *= 2;
            logger(0, LOG_LEVEL, MY_SERVER_ID, "New Progress timeout %d\n", PROGRESS_TIMEOUT);
            // set the progress timer
            gettimeofday(&PROGRESS_TIMER, NULL);
            PROGRESS_TIMER_SET = true;
            logger(0, LOG_LEVEL, MY_SERVER_ID, "Set Progress timer\n");
            // if leader of LAST_ATTEMPTED
            if ((LAST_ATTEMPTED % NUM_PEERS) == MY_SERVER_ID)
            {
                shift_to_prepare_phase();
            }
        }
    }
}

void received_vc_proof(VC_Proof *v)
{
    if (v->installed > LAST_INSTALLED)
    {
        LAST_ATTEMPTED = v->installed;
        if ((LAST_ATTEMPTED % NUM_PEERS) == MY_SERVER_ID)
        {
            shift_to_prepare_phase();
        }
        else
        {
            shift_to_reg_non_leader();
        }
    }
}
