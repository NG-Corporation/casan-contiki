#include "retrans.h"

/*Destructor*/
void freeRetrans(Retrans *rt) {
	free(rt->retransq_);
	free(rt);
}

Retrans *initRetrans (void) 
{
	Retrans *rt = (Retrans *) malloc (sizeof(Retrans));
	if (rt == NULL)
		printf("Memory allocation failed\n");
    rt->retransq_ = NULL ;
    return rt;
}


void resetRetrans (Retrans *rt) 
{
    while (rt->retransq_ != NULL)
		delRetransIntern2 (rt, NULL, rt->retransq_) ;
}


void master (Retrans *rt, l2addr_154 **master)
{
    rt->master_addr_ = master ;
}


// insert a new message in the retransmission list
void addRetrans (Retrans *rt, Msg *msg) 
{
    retransq *n ;

    sync_time (&curtime) ;		// synchronize curtime

    n = (retransq *) malloc (sizeof (retransq)) ;
    if (n == NULL)
		printf("Memory allocation failed\n");
    n->msg = msg ;
    n->timelast = curtime ;
    n->timenext = curtime + ALEA (ACK_TIMEOUT * ACK_RANDOM_FACTOR) ;
    n->ntrans = 0 ;
    n->next = rt->retransq_ ;
    rt->retransq_ = n ;
}


void delRetrans (Retrans *rt, Msg *msg) 
{
    delRetransIntern1 (rt, getRetrans (rt, msg)) ;
}


// TODO
void loopRetrans (Retrans *rt, l2net_154 *l2, time_t *curtime)
{
    // DBGLN1 (F ("retransmit loop")) ;

    if (*rt->master_addr_ == NULL)
    {
		// master has been reset by the CASAN engine (waiting-unknown state)
		resetRetrans (rt) ;
		return ;
    }
    else
    {
		retransq *cur, *prev ;

		prev = NULL ;
		cur = rt->retransq_ ;
		while (cur != NULL)
		{
		    retransq *next ;

		    next = cur->next ;
		    if (cur->ntrans >= MAX_RETRANSMIT)
		    {
				// remove the message from the queue
				delRetransIntern2 (rt, prev, cur) ;
				// prev is not modified
		    }
		    else
		    {
				if (cur->timenext < *curtime)
				{
				    sendMsg (cur->msg, *rt->master_addr_) ;
				    cur->ntrans++ ;
				    cur->timenext = cur->timenext + (2* (cur->timenext - cur->timelast));
				    sync_time (&cur->timelast) ;
				}
				prev = cur ;
		    }
		    cur = next ;
		}
    }
}


void check_msg_received (Retrans *rt, Msg *in) 
{
    switch (get_type (in))
    {
	case COAP_TYPE_ACK :
	    delRetrans (rt, in) ;
	    break ;
	default :
	    break ;
    }
}



void check_msg_sent (Retrans *rt, Msg *in) 
{
    switch (get_type (in))
    {
	case COAP_TYPE_CON :
	    addRetrans (rt, in) ;
	    break ;
	default :
	    break ;
    }
}


// for internal use only
void delRetransIntern1 (Retrans *rt, retransq *r) 
{
    retransq *cur, *prev ;

    prev = NULL ;
    for (cur = rt->retransq_ ; cur != NULL ; cur = cur->next)
    {
		if (cur == r)
		    break ;
		prev = cur ;
    }

    if (cur != NULL)
	delRetransIntern2 (rt, prev, cur) ;
}



// for internal use only
void delRetransIntern2 (Retrans *rt, retransq *prev, retransq *cur) 
{
    if (prev == NULL)
		rt->retransq_ = cur->next ;
    else
		prev->next = cur->next ;
    if (cur->msg != NULL)
		freeMsg (cur->msg) ;
    free (cur) ;
}



// get a message to retransmit, given its message id
retransq *getRetrans (Retrans *rt, Msg *msg) 
{
    retransq *cur ;

    for (cur = rt->retransq_ ; cur != NULL ; cur = cur->next)
    {
	// TODO : maybe check the token too
	if (get_id (cur->msg) == get_id (msg))
	    break ;
    }
    return cur ;
}