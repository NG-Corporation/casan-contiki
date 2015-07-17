#ifndef __RETRANS_H__
#define __RETRANS_H__

/*
 * Retransmission handling
 *
 * This class provides support for a list of messages to retransmit
 * The loop function must be called periodically in order to
 * retransmit and/or expire messages.
 */

#include "msg.h"
#include "time.h"

#define DEFAULT_TIMER 4000


typedef struct retransq
{
    Msg *msg ;
    time_t timelast ;		// time of last transmission
    time_t timenext ;		// time of next transmission
    uint8_t ntrans ;		// # of retransmissions 
    struct retransq *next ;		// next in queue
} retransq;


typedef struct retrans {
	retransq *retransq_ ;
	l2addr_154 **master_addr_ ;
}Retrans;


void freeRetrans(Retrans *rt);

Retrans *initRetrans (void);

void resetRetrans (Retrans *rt) ;

void master (Retrans *rt, l2addr_154 **master);

void addRetrans (Retrans *rt, Msg *msg) ;

void delRetrans (Retrans *rt, Msg *msg);

void loopRetrans (Retrans *rt, l2net_154 *l2, time_t *curtime);

void check_msg_received (Retrans *rt, Msg *in);

void check_msg_sent (Retrans *rt, Msg *in) ;

void delRetransIntern1 (Retrans *rt, retransq *r);

void delRetransIntern2 (Retrans *rt, retransq *prev, retransq *cur);

retransq *getRetrans (Retrans *rt, Msg *msg);


#endif