#ifndef CONMSG_H_
#define CONMSG_H_

/*extern "C" {
#include "config.h"
#include "radio.h"
#include "board.h"
#include "transceiver.h"
}*/


#include "contiki.h"
#include "stdbool.h"
#include "netstack.h"


#define	DEFAULT_MSGBUF_SIZE	10
#define MAX_PAYLOAD 125

/** Macro to help write uint16_t (such as addr2 or panid) constants */
#define	CONST16(lo,hi)	(((hi) << 8) | (lo))


#define	Z_GET_FRAMETYPE(w)	(((w)>>0) & 0x7)
#define	Z_SET_FRAMETYPE(n)	(n)
#define	 Z_FT_BEACON			0
#define	 Z_FT_DATA			1
#define  Z_FT_ACK			2
#define  Z_FT_MACCMD			3
#define	Z_GET_SEC_ENABLED(w)	(((w)>>3) & 0x1)
#define	Z_SET_SEC_ENABLED(n)	((n)<<3)
#define	Z_GET_FRAME_PENDING(w)	(((w)>>4) & 0x1)
#define	Z_SET_FRAME_PENDING(n)	((n)<<4)
#define	Z_GET_ACK_REQUEST(w)	(((w)>>5) & 0x1)
#define	Z_SET_ACK_REQUEST(n)	((n)<<5)
#define	Z_GET_INTRA_PAN(w)	(((w)>>6) & 0x1)
#define	Z_SET_INTRA_PAN(n)	((n)<<6)
#define	Z_GET_RESERVED(w)	(((w)>>7) & 0x3)
#define	Z_SET_RESERVED(n)	((n)<<7)
#define	Z_GET_DST_ADDR_MODE(w)	(((w)>>10) & 0x3)
#define	Z_SET_DST_ADDR_MODE(n)	((n)<<10)
#define	 Z_ADDRMODE_NOADDR		0
#define	 Z_ADDRMODE_RESERVED		1
#define	 Z_ADDRMODE_ADDR2		2
#define	 Z_ADDRMODE_ADDR8		3
#define	Z_GET_FRAME_VERSION(w)	(((w)>>12) & 0x3)
#define	Z_SET_FRAME_VERSION(n)	((n)<<12)
#define	 Z_FV_2003			0
#define	 Z_FV_2006			1
#define	 Z_FV_RESERVED2			2
#define	 Z_FV_RESERVED3			3
#define	Z_GET_SRC_ADDR_MODE(w)	(((w)>>14) & 0x3)
#define	Z_SET_SRC_ADDR_MODE(n)	((n)<<14)




	typedef int8_t channel_t;
	typedef uint16_t panid_t ;


	typedef uint16_t addr2_t ;
	typedef uint64_t addr8_t ;


	typedef struct ConReceivedFrame
	{
	    uint8_t frametype ;		///< Z_FT_DATA, Z_FT_ACK, etc.
	    uint8_t *rawframe ;		///< Pointer to frame header in reception buffer (ZigBuf)
	    uint8_t rawlen ;		///< Frame length
	    uint8_t lqi ;		///< Link Quality Indicator
	    uint8_t *payload ;		///< Pointer to 802.15.5 payload in reception buffer
	    uint8_t paylen ;		///< Payload length (without frame footer)
	    /** @brief Frame Control Field
	     *
	     * The Frame Control Field is the first field (16 bits) of
	     * the 802.15.4 header. Other items in the ZigReceivedFrame
	     * structure are decoded from this field.
	     *
	     * Don't display this field as an integer, since you will
	     * be fooled by the bit order. Use Z_* macros instead.
	     */
	    uint16_t fcf ;
	    uint8_t seq ;		///< Sequence number
	    addr2_t dstaddr ;		///< Destination address
	    addr2_t dstpan ;		///< Destination PAN id
	    addr2_t srcaddr ;		///< Source address
	    addr2_t srcpan ;		///< Source PAN id (same as dstpan if IntraPAN bit is set in FCF)
	} ConReceivedFrame;



	typedef struct ConStat
	{
	    int rx_overrun ;
	    int rx_crcfail ;
	    int tx_sent ;
	    int tx_error_cca ;
	    int tx_error_noack ;
	    int tx_error_fail ;
	} ConStat;


	


	typedef struct ConBuf
	{
	    volatile uint8_t frame [MAX_PAYLOAD] ;
	    volatile uint8_t len ;
	    volatile uint8_t lqi ;
	} ConBuf ;


	ConStat stat_ ;

	channel_t chan_ ;
	panid_t panid_ ;
	addr2_t addr2_ ;
	addr8_t addr8_ ;

	ConBuf *rbuffer_ ;		// with msgbufsize_ entries
	volatile int rbuffirst_ ;
	volatile int rbuflast_ ;
	int msgbufsize_ ;
	ConReceivedFrame rframe_ ;	// decoded received frame

		/*
	 * Transmission
	 */

	uint8_t seqnum_ ;		// to be placed in MAC header
	volatile bool writing_ ;


	ConStat *getstat (void) ; 

	void init(void);

	/** Accessor method to get the size (in number of frames) of the receive buffer */
	int getMsgbufsize (void) ; 

	/** Accessor method to get the channel id (11 ... 26) */
	channel_t getChannel (void) ;

	/** Accessor method to get our 802.15.4 hardware address (16 bits) */
	addr2_t getAddr2 (void)  ; 

	/** Accessor method to get our 802.15.4 hardware address (64 bits) */
	addr8_t getAddr8 (void) ; 

	/** Accessor method to get our 802.15.4 PAN id */
	panid_t getPanid (void); 

	/** Accessor method to get the TX power (-17 ... +3 dBM) */
	//txpwr_t txpower (void) { return txpower_ ; }	// -17 .. +3 dBm

	/** Accessor method to get promiscuous status */
	//bool promiscuous (void) { return promisc_ ; }

	/** Mutator method to set the size (in number of frames) of the receive buffer */
	void setMsgbufsize (int msgbufsize) ; 

	/** Mutator method to set the channel id (11 ... 26) */
	void setChannel (channel_t chan) ;	// 11..26

	/** Mutator method to set our 802.15.4 hardware address (16 bits) */
	void setAddr2 (addr2_t addr) ; 

	/** Mutator method to set our 802.15.4 hardware address (16 bits) */
	void setAddr8 (addr8_t addr)  ; 

	/** Mutator method to set our 802.15.4 PAN id */
	void setPanid (panid_t panid)  ; 

	/** Mutator method to set the TX power (-17 ... +3 dBM) */
	//void txpower (txpwr_t txpower) { txpower_ = txpower ; }

	/** Mutator method to set promiscuous status */
	//void promiscuous (bool promisc) { promisc_ = promisc ; }

	// Start radio processing

	void start (void) ;


		// Not really public: interrupt functions are designed to
	// be called outside of an interrupt
	uint8_t *usr_radio_receive_frame (uint8_t len, uint8_t *frm) ;
	void usr_radio_tx_done (void) ;

	// Send and receive frames

	bool sendto (addr2_t a,  const uint8_t payload [], uint8_t len) ;
	ConReceivedFrame *get_received (void) ;	// get current frame (or NULL)
	void skip_received (void) ;	// skip to next read frame

	
	uint16_t addr_trad (const char *a);
	/**
	 * Return operational statistics
	 *
	 * Note that returned ZigMsg::ZigStat structure may still be
	 * modified by an interrupt routine.
	 */
	

#endif