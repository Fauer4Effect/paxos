#include <stdint.h>

#include "messages.h"

void packi32(unsigned char *buf, uint32_t i);
uint32_t unpacki32(unsigned char *buf);

void pack_header(Header *header, unsigned char *buf);
void unpack_header(Header *header, unsigned char *buf);

void pack_client_update(Client_Update *msg, unsigned char *buf);
void unpack_client_update(Client_Update *msg, unsigned char *buf);

void pack_view_change(View_Change *msg, unsigned char *buf);
void unpack_view_change(View_Change *msg, unsigned char *buf);

void pack_vc_proof(VC_Proof *msg, unsigned char *buf);
void unpack_vc_proof(VC_Proof *msg, unsigned char *buf);

void pack_prepare(Prepare *msg, unsigned char *buf);
void unpack_prepare(Prepare *msg, unsigned char *buf);

void pack_prepare_ok(Prepare_OK *msg, unsigned char *buf);
void unpack_prepare_ok(Prepare_OK *msg, unsigned char *buf);

void pack_proposal(Proposal *msg, unsigned char *buf);
void unpack_proposal(Proposal *msg, unsigned char *buf);

void pack_accept(Accept *msg, unsigned char *buf);
void unpack_accept(Accept *msg, unsigned char *buf);

void pack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf);
void unpack_global_ordered(Globally_Ordered_Update *msg, unsigned char *buf);