#ifndef _DATAH_
#define _DATAH_

#include "messages.h"
#include "logging.h"

//#define (LEADER_ELECTION) (1);
//#define (REG_LEADER) (2);
//#define (REG_NONLEADER) (3);
int LEADER_ELECTION = 1;
int REG_LEADER = 2;
int REG_NONLEADER = 3;

extern int MAX_CLIENT_ID;
extern int LOG_LEVEL;
extern int MY_SERVER_ID;


#endif
