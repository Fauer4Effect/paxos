#include <stdlib.h>

#include "data_structures.h"
#include "prepare.h"
#include "messages.h"
#include "serialize.h"
#include "multicast.h"

// datalist_storage_reqs = num nodes * storage of each node
// a node has data_type + pointer to next + actual storage of the data
// data is either globally_ordered_update or proposal
// globally_ordered_update is uint32_t * 3
// proposal has the uin32_t * 3 + client_update
// client_update is uin32_t * 4
int datalist_storage_reqs(node_t *datalist)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Computing datalist storage reqs\n");
    int size = 0;
    if (datalist == NULL)
        logger(0, LOG_LEVEL, MY_SERVER_ID, "Data list is null\n");
    int num_nodes = list_length(datalist);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Got list length\n");
    int i;
    for (i = 0; i < num_nodes; i++)
    {
        size += (sizeof(uint32_t) + sizeof(node_t *));
        if (datalist->data_type == Globally_Ordered_Update_Type)
        {
            size += (sizeof(uint32_t) * 3);
        }
        else if (datalist->data_type == Proposal_Type)
        {
            size += (sizeof(uint32_t) * 7);
        }
        datalist = datalist->next;
    }
    return size;
}

node_t *construct_datalist(int aru)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Constructing datalist\n");
    node_t *datalist = malloc(sizeof(node_t));
    int i;
    for (i = aru + 1; i < MAX_CLIENT_ID; i++)
    {
        if (GLOBAL_HISTORY[i] == 0)
        {
            continue;
        }
        if (GLOBAL_HISTORY[i]->global_ordered_update != 0)
        {
            append_to_list(GLOBAL_HISTORY[i]->global_ordered_update, datalist,
                           Globally_Ordered_Update_Type);
        }
        else
        {
            append_to_list(GLOBAL_HISTORY[i]->proposal, datalist, Proposal_Type);
        }
    }
    return datalist;
}

bool view_prepared_ready(int view)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Check if view prepared ready\n");
    int i;
    int count;
    for (i = 0; i < NUM_PEERS; i++)
    {
        if (PREPARE_OKS[i]->view == view)
            count++;
    }
    if (count >= ((NUM_PEERS / 2) + 1))
        return true;
    return false;
}

void shift_to_prepare_phase()
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Shift to prepare phase\n");
    LAST_INSTALLED = LAST_ATTEMPTED;
    Prepare *prepare = malloc(sizeof(Prepare));
    prepare->server_id = MY_SERVER_ID;
    prepare->view = LAST_INSTALLED;
    prepare->local_aru = LOCAL_ARU;
    // Apply prepare to data structures
    apply_prepare(prepare);

    node_t *datalist = construct_datalist(LOCAL_ARU);

    Prepare_OK *prepare_ok = malloc(sizeof(Prepare_OK));
    prepare_ok->server_id = MY_SERVER_ID;
    prepare_ok->view = LAST_INSTALLED;
    prepare_ok->data_list = datalist;

    PREPARE_OKS[MY_SERVER_ID] = prepare_ok;

    int i;
    for (i = 0; i < MAX_CLIENT_ID; i++)
    {
        LAST_ENQUEUED[i] = 0;
    }

    // XXX Sync to disk

    unsigned char *prepare_buf = malloc(sizeof(Prepare));
    pack_prepare(prepare, prepare_buf);

    Header *header = malloc(sizeof(Header));
    header->msg_type = Prepare_Type;
    header->size = sizeof(Prepare);
    unsigned char *header_buf = malloc(sizeof(Header));
    pack_header(header, header_buf);

    multicast(header_buf, sizeof(Header), prepare_buf, header->size);

    free(prepare_buf);
    free(header_buf);
    free(header);
}

void received_prepare(Prepare *prepare)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received prepare\n");
    Prepare_OK *prepare_ok;
    int leader_id = prepare->view % NUM_PEERS;
    if (STATE == LEADER_ELECTION)
    {
        apply_prepare(prepare);

        node_t *datalist = construct_datalist(prepare->local_aru);

        prepare_ok = malloc(sizeof(Prepare_OK));
        prepare_ok->server_id = MY_SERVER_ID;
        prepare_ok->view = prepare->view;
        prepare_ok->data_list = datalist;
        PREPARE_OKS[MY_SERVER_ID] = prepare_ok;

        shift_to_reg_non_leader();
    }
    else
    {
        // already installed the view
        prepare_ok = PREPARE_OKS[MY_SERVER_ID];
    }

    // XXX Send to leader prepare_ok
    Header *header = malloc(sizeof(Header));
    header->msg_type = Prepare_OK_Type;
    // server_id + view + size_of_datalist + datalist_storage_reqs
    header->size = (sizeof(uint32_t) * 3) + datalist_storage_reqs(prepare_ok->data_list);
    unsigned char *header_buf = malloc(sizeof(Header));
    pack_header(header, header_buf);

    unsigned char *prepare_ok_buf = malloc(header->size);
    pack_prepare_ok(prepare_ok, prepare_ok_buf);
    send_to_single_host(header_buf, sizeof(Header), prepare_ok_buf, header->size, leader_id);

    free(prepare_ok_buf);
    free(header_buf);
    free(header);
}

void received_prepare_ok(Prepare_OK *prepare_ok)
{
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received prepare ok\n");
    apply_prepare_ok(prepare_ok);
    if (view_prepared_ready(prepare_ok->view))
    {
        shift_to_reg_leader();
    }
}
