/**
 * @file l2-154.h
 * @brief l2addr and l2net specializations for IEEE 802.15.4
 */

#ifndef L2_154_H
#define	L2_154_H

/*
 * These classes are used for IEEE 802.15.4 specialization
 */


#include "../ConMsg/ConMsg.h"
#include "../Casan/defs.h"
#include <stddef.h> 


#define	I154_ADDRLEN	2		// Address length
#define	I154_MTU	127

	typedef struct l2addr_154 {
		addr2_t addr_;
	}l2addr_154;

	typedef enum
	{
	    RECV_EMPTY, 		///< No received message
	    RECV_WRONG_TYPE,		///< Wrong Ethernet type, for example
	    RECV_WRONG_DEST,		///< Wrong destination address
	    RECV_TRUNCATED,		///< Truncated message
	    RECV_OK			///< Message received successfully
	} l2_recv_t ;


	typedef struct l2net_154 {
		ConReceivedFrame *curframe_;
		addr2_t myaddr_;

		/** Current MTU value
		 *
		 * Derived classes must initialize this value to the network
		 * default MTU after object creation. It can be modified
		 * afterwards by the calling program, for example to reflect
		 * the result of an MTU negociation.
		 * Maximum payload length is derived from this value.
		 */
		size_t mtu_ ;		// must be initialized in derived classes
	}l2net_154;


	extern l2addr_154 *l2addr_154_broadcast;

	void freel2addr_154(l2addr_154 *addr);

	l2addr_154 *init_l2addr_154_char(const char*);

	l2addr_154 *init_l2addr_154_addr(const l2addr_154 *x);

	void copyAddr(l2addr_154 *x,const l2addr_154 *y);

	bool isEqualAddr(const l2addr_154 *a1, const l2addr_154 *a2);

	void printAddr (const l2addr_154 *x) ;

	//void startL2_154 (l2net_154 *l2, l2addr_154 *myaddr, channel_t chan, panid_t panid) ;

	l2net_154* startL2_154 ( l2addr_154 *a, channel_t chan, panid_t panid);

	size_t maxpayload (l2net_154 *l2) ;

	bool send (l2net_154 *l2, l2addr_154 *dest, const uint8_t *data, size_t len) ;

	void setBroadcastAddr(void);

	void setMTU(l2net_154 *l2, size_t mtu);

	size_t getMTU(l2net_154 *l2);

	// the "recv" method copy the received packet in
	// the instance private variable (see rbuf_/rbuflen_ below)
	l2_recv_t recv (l2net_154 *l2) ;

	l2addr_154 *bcastaddr (void) ;	// return a static variable
	l2addr_154 *get_src (l2net_154 *l2) ;	// get a new l2addr_154
	l2addr_154 *get_dst (l2net_154 *l2) ;	// get a new l2addr_154

	// Payload (not including MAC header, of course)
	uint8_t *get_payload (l2net_154 *l2,int offset) ;
	size_t get_paylen (l2net_154 *l2) ;	// if truncated pkt: truncated payload

	// debug usage
	void dump_packet (l2net_154 *l2, size_t start, size_t maxlen) ;

	

#endif

