/**
 * @file l2.h
 * @brief l2addr and l2net virtual class interfaces
 */

#ifndef	L2_H
#define	L2_H


 	typedef enum
	{
	    RECV_EMPTY, 		///< No received message
	    RECV_WRONG_TYPE,		///< Wrong Ethernet type, for example
	    RECV_WRONG_DEST,		///< Wrong destination address
	    RECV_TRUNCATED,		///< Truncated message
	    RECV_OK			///< Message received successfully
	} l2_recv_t ;


#endif