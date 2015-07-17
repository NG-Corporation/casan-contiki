/**
 * @file debug.cpp
 * @brief Debug class implementation
 */

#include "debug.h"

int freeMemory () ;

/**
 * @brief Initializes the debug facility
 *
 * This method is designed to be called in the application `setup`
 * function.
 *
 * @param interval interval (in seconds) between heartbeats
 */

Debug *startDebug (int interval)	// in seconds
{
	Debug *de = (Debug *) malloc(sizeof(Debug));
    printf ("%s",B_BLUE ("start") ) ;
    de->interv_ = interval * 1000 ;		// in milliseconds
    de->next_ = clock_time() ;			// perform action immediately
    return de;
}


/**
 * @brief Signals that the heartbeat interval has been reached
 *
 * This method is designed to be called in the application `loop`
 * function. When the heartbeat interval has been reached, it
 * displays a message including the amount of free memory
 * in order to detect memory leaks, and returns `true` so that
 * the application `loop` can perform additional (periodic) tasks.
 *
 * @return true if the heartbeat interval has been reached
 */

bool heartbeatDebug (Debuf *de)
{
    bool action = false ;

    if (clock_time() > de->next_)
    {
		printf ("-------------------------------------------------------------------") ;
		printf ("%s free mem = ",B_BLUE ("loop ")) ;
		DBG1 (freeMemory ()) ;
		printf("\n");

		de->next_ += de->interv_ ;
		action = true ;
    }

    return action ;
}