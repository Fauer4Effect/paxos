#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "messages.h"

/*
Pack an integer into the buffer
*/
void packi32(unsigned char *buf, uint32_t i)
{
    *buf++ = i >> 24;
    *buf++ = i >> 16;
    *buf++ = i >> 8;
    *buf++ = i;
}

/*
Unpack an integer from the buffer
*/
uint32_t unpacki32(unsigned char *buf)
{
    uint32_t i = ((uint32_t)buf[0] << 24) |
                 ((uint32_t)buf[1] << 16) |
                 ((uint32_t)buf[2] << 8) |
                 buf[3];
    return i;
}

void pack_header(Header *header, unsigned char *buf)
{
    packi32(buf, header->msg_type);
    buf += 4;
    packi32(buf, header->size);
}

void unpack_header(Header *header, unsigned char *buf)
{
    header->msg_type = unpacki32(buf);
    buf += 4;
    header->size = unpacki32(buf);
}

void pack_client_update(Client_Update *msg, unsigned char *buf)
{
    packi32(buf, msg->client_id);
    buf += 4;
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->timestamp);
    buf += 4;
    packi32(buf, msg->update);
}

void unpack_client_update(Client_Update *msg, unsigned char *buf)
{
    msg->client_id = unpacki32(buf);
    buf += 4;
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->timestamp = unpacki32(buf);
    buf += 4;
    msg->update = unpacki32(buf);
}

void pack_view_change(View_Change *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->attempted);
}

void unpack_view_change(View_Change *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->attempted = unpacki32(buf);
}

void pack_vc_proof(VC_Proof *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->installed);
}

void unpack_vc_proof(VC_Proof *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->installed = unpacki32(buf);
}

void pack_prepare(Prepare *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;
    packi32(buf, msg->local_aru);
}

void unpack_prepare(Prepare *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;
    msg->local_aru = unpacki32(buf);
}

void pack_proposal(Proposal *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;
    packi32(buf, msg->seq);
    buf += 4;

    pack_client_update(msg->update, buf);
}

void unpack_proposal(Proposal *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;
    msg->seq = unpacki32(buf);
    buf += 4;

    Client_Update *update = malloc(sizeof(Client_Update));
    unpack_client_update(update, buf);
    msg->update = update;
}

void pack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->seq);
    buf += 4;

    pack_client_update(msg->update, buf);
}

void unpack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->seq = unpacki32(buf);
    buf += 4;

    Client_Update *update = malloc(sizeof(Client_Update));
    unpack_client_update(update, buf);
    msg->update = update;
}
void pack_prepare_ok(Prepare_OK *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;

    int datalist_size = list_length(msg->data_list);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Data list size is %d\n", datalist_size);
    packi32(buf, datalist_size);
    buf += 4;

    // datalist is msg->size nodes in size
    // for each node in the datalist we need to pack it based on
    // its type
    int stored = 0;
    node_t *datalist = msg->data_list;
    while (stored < datalist_size)
    {
        // each node has void *data, node *next, uint32_t data_type
        packi32(buf, datalist->data_type);
        buf += 4;
        if (datalist->data_type == Globally_Ordered_Update_Type)
        {
            pack_global_ordered(datalist->data, buf);
            buf += 4; // carry over from pack_global_ordered
        }
        else if (datalist->data_type == Proposal_Type)
        {
            pack_proposal(datalist->data, buf);
            buf += 4; // carry over  from pack_client_update
        }
        datalist = datalist->next; // advance datalist
        stored++;
    }
}

void unpack_prepare_ok(Prepare_OK *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;

    node_t *datalist = malloc(sizeof(node_t));
    datalist->data = NULL;
    datalist->next = NULL;
    datalist->data_type = 0;

    int datalist_size = unpacki32(buf);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Received data list size %d\n", datalist_size);
    buf += 4;

    int index = 0;
    while (index < datalist_size)
    {
        int data_type = unpacki32(buf);
        buf += 4;

        if (data_type == Globally_Ordered_Update_Type)
        {
            Globally_Ordered_Update *update = malloc(sizeof(Globally_Ordered_Update));
            unpack_global_ordered(update, buf);
            buf += 4; // carry over from unpack_global_ordered

            append_to_list(update, datalist, data_type);
        }
        else if (data_type == Proposal_Type)
        {
            Proposal *proposal = malloc(sizeof(Proposal));
            unpack_proposal(proposal, buf);
            buf += 4; // carry over from unpack_proposal

            append_to_list(proposal, datalist, data_type);
        }

        index++;
    }

    msg->data_list = datalist;
}

void pack_accept(Accept *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;
    packi32(buf, msg->seq);
}

void unpack_accept(Accept *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;
    msg->seq = unpacki32(buf);
}
