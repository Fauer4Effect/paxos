#ifndef _MESSAGESH_
#define _MESSAGESH_

#include <stdint.h>

#include "data_structures.h"

// define message types
#define Client_Update_Type 1
#define View_Change_Type 2
#define VC_Proof_Type 3
#define Prepare_Type 4
#define Prepare_OK_Type 5
#define Proposal_Type 6
#define Accept_Type 7
#define Globally_Ordered_Update_Type 8

// XXX right now we are not doing reconciliation so we won't have any globally_ordered_updates
// if we were doing this for real I would probably break this into two separate data_lists
typedef struct
{
    uint32_t size;
    Proposal *proposals[];
} datalist_t;

typedef struct
{
    uint32_t msg_type;
    uint32_t size;
} Header;

typedef struct
{
    uint32_t client_id; // identifier of the sending client
    uint32_t server_id; // identifier of this client's server
    uint32_t timestamp; // client sequence number for this update
    uint32_t update;    // update veing initiated by the client
} Client_Update;

typedef struct
{
    uint32_t server_id; // identifier of the sending server
    uint32_t attempted; // view number this server is trying to install
} View_Change;

typedef struct
{
    uint32_t server_id; // identifier of the sending server
    uint32_t installed; // last view number this server installed
} VC_Proof;

typedef struct
{
    uint32_t server_id; // identifier of the sending server
    uint32_t view;      // view number being prepared
    uint32_t local_aru; // local aru value of the leader
} Prepare;

typedef struct
{
    uint32_t server_id; // identifier of the sending server
    uint32_t view;      // view number for which this message applies
    node_t data_list;   // list of Proposals and Globally_Ordered_Updates
    // uint32_t size;
    // datalist_t *data_list;
    // Globally_Ordered_Update *data_list;     // list of Proposals and Globally_Ordered_Updates
    // uint32_t data_list_size;                // how many entries in the data_list
    // Proposal *data_list[];                  // list of Proposals
} Prepare_OK;

typedef struct
{
    uint32_t server_id;    // identifier of the sending server
    uint32_t view;         // view in which this proposal is being made
    uint32_t seq;          // sequence number of this proposal
    Client_Update *update; // client update being bound to seq in this proposal
} Proposal;

typedef struct
{
    uint32_t server_id; // identifier of the sending server
    uint32_t view;      // view for which this message applies
    uint32_t seq;       // sequence number of the associated Proposal
} Accept;

// ??? this is used only for reconciliation
typedef struct
{
    uint32_t server_id;    // identifier of the sending server
    uint32_t seq;          // sequence number of the update that was ordered
    Client_Update *update; // client update bound to the seq and globally ordered
} Globally_Ordered_Update;

#endif