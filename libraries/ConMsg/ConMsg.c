#include "ConMsg.h"




#define	Z_SET_INT16(p,val) \
			(((uint8_t *) p) [0] = ((val)>>0) & 0xff, \
			 ((uint8_t *) p) [1] = ((val)>>8) & 0xff )
#define	Z_GET_INT16(p)	( (((uint8_t *) p) [0] << 0) | \
			  (((uint8_t *) p) [1] << 8) )

#define I154_ADDRLEN 2


ConStat *getstat (void) { return &stat_ ; }

int getMsgbufsize (void) { return msgbufsize_ ; }

addr2_t getAddr2 (void) { return addr2_ ; }

addr8_t getAddr8 (void) { return addr8_ ; }

panid_t getPanid (void) { return panid_ ; }

channel_t getChannel (void) { return chan_ ; }

void setMsgbufsize (int msgbufsize) { msgbufsize_ = msgbufsize ; }

void setAddr2 (addr2_t addr) { addr2_ = addr ; }

void setAddr8 (addr8_t addr) { addr8_ = addr ; }

void setPanid (panid_t panid) { panid_ = panid ; }

void setChannel (channel_t chan) { chan_ = chan ; }

uint8_t *usr_radio_receive_frame (uint8_t len, uint8_t *frm)
{
	
	int newlast = (rbuflast_ + 1) % msgbufsize_ ;
	if (newlast == rbuffirst_)
	    stat_.rx_overrun++ ;
	else
	{
	    /*
	     * There is room for a new frame:
	     * - update the message with length and lqi
	     * - update the frame buffer pointer
	     */

	    rbuffer_ [rbuflast_].len = len ;

	    rbuflast_ = newlast ;
	    frm = (uint8_t *) rbuffer_ [newlast].frame ;
	}
    return frm;
}



void usr_radio_tx_done ()
{
	
    writing_ = false ;
}



void init(void) {
	chan_ = 17;
	writing_ = false;
}


void start(void) {

	if (msgbufsize_ == 0)		// prevent stupid errors...
	msgbufsize_ = DEFAULT_MSGBUF_SIZE ;

	if (rbuffer_ != NULL)
		free(rbuffer_) ;
    rbuffer_ = (ConBuf *)malloc(sizeof(struct ConBuf)*msgbufsize_) ;
    rbuffirst_ = 0 ;
    rbuflast_ = 0 ;


    //stat_ = (ConStat)malloc(sizeof(conStat));

    writing_ = false;
    seqnum_ = 0;

    NETSTACK_RADIO.init();
    initBuf((uint8_t *) rbuffer_ [rbuflast_].frame, MAX_PAYLOAD);
    NETSTACK_RADIO.on();

}



bool sendto ( addr2_t a,  const uint8_t payload[], uint8_t len ) {
	uint8_t frame[MAX_PAYLOAD];
	uint16_t fcf ;
	int frmlen ;

	frmlen = 9 + len + 2;
	if(frmlen > MAX_PAYLOAD)
		return false;



	fcf = Z_SET_FRAMETYPE (Z_FT_DATA)
	    | Z_SET_SEC_ENABLED (0)
	    | Z_SET_FRAME_PENDING (0)
	    | Z_SET_ACK_REQUEST (1)
	    | Z_SET_INTRA_PAN (1)
	    | Z_SET_RESERVED (0)
	    | Z_SET_DST_ADDR_MODE (Z_ADDRMODE_ADDR2)
	    | Z_SET_FRAME_VERSION (Z_FV_2003)
	    | Z_SET_SRC_ADDR_MODE (Z_ADDRMODE_ADDR2)
	    ;

	//Exemple
	    Z_SET_INT16 (&frame [0], fcf) ;		// fcf
    frame [2] = ++seqnum_ ; ;			// seq
    Z_SET_INT16 (&frame [3], panid_) ;		// dst panid
    Z_SET_INT16 (&frame [5], a) ;		// dst addr
    Z_SET_INT16 (&frame [7], addr2_) ;		// src addr

    memcpy (frame + 9, payload, len) ;
    writing_ = true ;
    NETSTACK_RADIO.send (frame, frmlen) ;

    while (writing_);

	return true;
}


ConReceivedFrame *get_received (void) {
	ConReceivedFrame *r ;
    ConBuf *b ;

    platform_enter_critical();
    if (rbuffirst_ == rbuflast_) {
		b = NULL ;
	}
    else b = & rbuffer_ [rbuffirst_] ;
    platform_exit_critical();

    if (b == NULL)
		r = NULL ;
    else
    {
    	uint8_t *p = (uint8_t *) b->frame ;
		int intrapan ;

		r = & rframe_ ;
		r->rawframe = (uint8_t *) b->frame ;
		r->rawlen = b->len ;

		/* decode Frame Control Field */
		r->fcf = Z_GET_INT16 (p) ;
		p += 2 ;

		r->frametype = Z_GET_FRAMETYPE (r->fcf) ;

		/* Sequence Number */
		r->seq = *p++ ;

		/* Dest & Src address */
		intrapan = Z_GET_INTRA_PAN (r->fcf) ;
		switch (Z_GET_DST_ADDR_MODE (r->fcf))
		{
		    case Z_ADDRMODE_NOADDR :
			break ;
		    case Z_ADDRMODE_RESERVED :
			break ;
		    case Z_ADDRMODE_ADDR2 :
			r->dstpan = Z_GET_INT16 (p) ;
			p += 2 ;
			r->dstaddr = Z_GET_INT16 (p) ;
			p += 2 ;
			break ;
		    case Z_ADDRMODE_ADDR8 :
			p += 8+2 ;
			break ;
		}
		switch (Z_GET_SRC_ADDR_MODE (r->fcf))
		{
		    case Z_ADDRMODE_NOADDR :
			break ;
		    case Z_ADDRMODE_RESERVED :
			break ;
		    case Z_ADDRMODE_ADDR2 :
			if (intrapan != 0)
			    r->srcpan = r->dstpan ;
			else
			{
			    r->srcpan = Z_GET_INT16 (p) ;
			    p += 2 ;
			}
			r->srcaddr = Z_GET_INT16 (p) ;
			p += 2 ;
			break ;
		    case Z_ADDRMODE_ADDR8 :
			if (intrapan == 0)
			    p += 2 ;
			p += 8 ;
			break ;
		}

		r->payload = p ;
		r->paylen = r->rawlen - (p - r->rawframe) - 2 ;	// skip FCS
    }
    return r ;

}



void skip_received (void)
{
    platform_enter_critical();
    if (rbuffirst_ != rbuflast_)
	rbuffirst_ = (rbuffirst_ + 1) % msgbufsize_ ;
    platform_exit_critical();
}

