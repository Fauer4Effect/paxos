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
    uint32_t i = ((uint32_t) buf[0]<<24) |
                 ((uint32_t) buf[1]<<16) |
                 ((uint32_t) buf[2]<<8)  |
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

void pack_prepare_ok(Prepare_OK *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;
    packi32(buf, msg->data_list_size);
    buf += 4;

    int index = 0;
    int stored = 0;
    while (stored < msg->data_list_size)
    {
        if (msg->data_list[index] != 0)
        {
            packi32(buf, msg->data_list[index]);
            buf += 4;
            stored++;
        }
        index++;
    }
}

void unpack_prepare_ok(Prepare_OK *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;
    msg->data_list_size = unpacki32(buf);
    buf += 4;

    int index = 0;
    while (index < msg->data_list_size)
    {
        msg->data_list[index] = unpacki32(buf);
        buf += 4;
        index++;
    }
}

// FIXME NEED TO FIX THE PACKING AND UNPACKING
void pack_proposal(Proposal *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->view);
    buf += 4;
    packi32(buf, msg->seq);
    buf += 4;
    packi32(buf, msg->update);
}

void unpack_proposal(Proposal *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->view = unpacki32(buf);
    buf += 4;
    msg->seq = unpacki32(buf);
    buf += 4;
    msg->update = unpacki32(buf);
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

void pack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf)
{
    packi32(buf, msg->server_id);
    buf += 4;
    packi32(buf, msg->seq);
    buf += 4;
    packi32(buf, msg->update);
}

void unpack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf)
{
    msg->server_id = unpacki32(buf);
    buf += 4;
    msg->seq = unpacki32(buf);
    buf += 4;
    msg->update = unpacki32(buf);
}