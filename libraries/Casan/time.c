/**
 * @file time.cpp
 * @brief implementation of time related classes
 */

#include "time.h"

/*
 * Timers values (expressed in ms)
 */

#define	TIMER_WAIT_START	1*500		// initial between discover msg
#define	TIMER_WAIT_INC		1*1000
#define	TIMER_WAIT_INC_MAX	10*1000
#define	TIMER_WAIT_MAX		30*1000		// time in waiting_known

#define	TIMER_RENEW_MIN		500		// min time between discover


/*
 * Macros to handle 64 bits integers
 */

#define	TIME_HIGH(t)		((uint32_t) (((t) >> 32) & 0xffffffff))
#define	TIME_LOW(t)		((uint32_t) (((t)      ) & 0xffffffff))
#define	MK_TIME(n,ms)		((((uint64_t) (n)) << 32) | ms)


/*******************************************************************************
 * Current time
 */

time_t curtime ;			// global variable

// synchronize current time with help of millis ()
// Should be used on curtime global variable, but can also be used on any var
void sync_time (time_t *cur)		
{
    unsigned long int ms ;
    uint32_t n ;

    n = TIME_HIGH (*cur) ;
    ms = clock_time() ;			// current time
    if (ms < TIME_LOW (*cur))		// rollover?
	n++ ;
    *cur = MK_TIME (n, ms) ;

}


void print_time (time_t *t)
{
    printf ("time = %d", TIME_HIGH (*t)) ;
    printf(":") ;
    printf ("%d\n",TIME_LOW (*t)) ;
}



/*******************************************************************************
 * Timers
 */

/*
 * twait: to send Discover messages in waiting_unknown and waiting_known
 *      states
 */

/** @brief Initialize the timer with the current time
 */

Twait *initTwait (time_t *cur)
{
	Twait *tw = (Twait *) malloc(sizeof(Twait));
    if (tw == NULL)
        printf("Memory allocation failed\n");
    tw->limit_ = *cur + TIMER_WAIT_MAX ;
    tw->inc_ = TIMER_WAIT_START ;
    tw->next_ = *cur + tw->inc_ ;
    return tw;
}


/** @brief Is it the time to send a new Discover message?
 */

bool nextTwait (Twait *tw, time_t *cur)
{
    bool itstime = false ;
    if (*cur >= tw->next_)
    {
		itstime = true ;
		tw->inc_ += TIMER_WAIT_INC ;
		if (tw->inc_ > TIMER_WAIT_INC_MAX)
		    tw->inc_ = TIMER_WAIT_INC_MAX ;
		tw->next_ += tw->inc_ ;
    }
    return itstime ;
}


/** @brief Is it the time for a transition from the waiting_known
 *	to the waiting_unknown state?
 */

bool expiredTwait (Twait *tw, time_t *cur)
{
    return *cur >= tw->limit_ ;
}


/** @brief Initialize the timer with the current time and the Slave TTL
 *	returned by the master in its Assoc message.
 */

Trenew *initTrenew ( time_t *cur, time_t sttl)
{
	Trenew *tr = (Trenew *) malloc (sizeof(Trenew));
    if (tr == NULL)
        printf("Memory allocation failed\n");
    tr->inc_ = sttl / 2 ;

    tr->next_ = *cur + tr->inc_ ;
    tr->limit_ = *cur + sttl ;
    return tr;
}


/** @brief Is it the time to enter the renew state and begin to send
 *	Discover messages?
 */

bool renewTrenew (Trenew *tr, time_t *cur)
{
    // printf("ici renew\n");
    // printf("%d    %d    %d\n",cur , &cur, *cur);
    return nextTrenew (tr, cur) ;
}


/** @brief Is it the time to send a new Discover message?
 */

bool nextTrenew (Trenew *tr, time_t *cur)
{
    bool itstime = false ;
    
    if (*cur >= tr->next_)
    {
		itstime = true ;
		tr->inc_ /= 2 ;
		if (tr->inc_ <= TIMER_RENEW_MIN)
		    tr->inc_ = TIMER_RENEW_MIN ;
		tr->next_ += tr->inc_ ;
    }
    return itstime ;
}


/** @brief Has association expired?
 */

bool expiredTrenew (Trenew *tr, time_t *cur)
{
    return *cur >= tr->limit_ ;
}