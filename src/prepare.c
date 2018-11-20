
#include "data_structures.h"
#include "prepare.h"
#include "messages.h"
#include "serialize.h"
#include "multicast.h"

node_t *construct_datalist(int aru)
{
    node_t *datalist = malloc(sizeof(node_t));
    int i;
    for (i = aru+1; i < MAX_CLIENT_ID; i++)
    {
        if (GLOBAL_HISTORY[i]->global_ordered_update != 0)
        {
            append_to_list(GLOBAL_HISTORY[i]->global_ordered_update, datalist);
        } else
        {
            append_to_list(GLOBAL_HISTORY[i]->proposal, datalist);
        }
    }
    return datalist;
}

bool view_prepared_ready(int view)
{
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (PREPARE_OKS[i]->view == view)
            count++;
    }
    if (count >= ((NUM_PEERS/2)+1))
        return true;
    return false;
}

void shift_to_prepare_phase()
{
    LAST_INSTALLED = LAST_ATTEMPTED;
    Prepare *prepare = malloc(sizeof(Prepare));
    prepare->server_id = MY_SERVER_ID;
    prepare->view = LAST_INSTALLED;
    prepare->local_aru = LOCAL_ARU;
    // ??? Apply prepare to data structures
    PREPARED = prepare;

    node_t *datalist = construct_datalist(LOCAL_ARU);

    Prepare_OK *prepare_ok = malloc(sizeof(Prepare_OK));
    prepare_ok->server_id = MY_SERVER_ID;
    prepare_ok->view = LAST_INSTALLED;
    prepare_ok->size = datalist->size;
    prepare_ok->data_list = datalist;

    PREPARE_OKS[MY_SERVER_ID] = prepare_ok;

    int i;
    for (i = 0; i < LAST_ENQUEUED_SIZE; i++)
    {
        free(LAST_ENQUEUED[i]);
        LAST_ENQUEUED[i] = 0;
    }

    // XXX Sync to disk

    // server_id + view + size + datalist
    // datalist is uint32_t + sizeof(Proposal) * prepare->size
    unsigned char *prepare_buf = malloc(sizeof(Prepare));
    pack_prepare(prepare, prepare_buf);

    Header *header = malloc(sizeof(Header));
    header->msg_type = Prepare_Type;
    header->size = sizeof(Prepare);
    unsigned char *header_buf = malloc(sizeof(Header));
    pack_header(header, header_buf);

    multicast(header_buf, sizeof(header_buf), prepare_buf, sizeof(prepare_buf));

    free(prepare_buf);
    free(prepare);
    free(header_buf);
    free(header);
}

void received_prepare(Prepare *prepare)
{
    if (STATE == LEADER_ELECTION)
    {
        // ??? apply prepare to data structures
        PREPARED = prepare;
        node_t *datalist = construct_datalist(prepare->local_aru);

        Prepare_OK *prepare_ok = malloc(sizeof(Prepare_OK));
        prepare_ok->server_id = MY_SERVER_ID;
        prepare_ok->view = prepare->view;
        prepare_ok->size = datalist->size;
        prepare_ok->data_list = datalist;
        PREPARE_OKS[MY_SERVER_ID] = prepare_ok;
        
        shift_to_reg_non_leader();

        // XXX Send to leader prepare_ok
    } else
    {
        // already installed the view
        // XXX send to leader Prepared_oks[my_server_id]
    }
}

void received_prepare_ok(Prepare_OK *prepare_ok)
{
    // XXX Apply to data structures
    if (view_prepared_ready(prepare_ok->view))
    {
        shift_to_reg_leader();
    }
}