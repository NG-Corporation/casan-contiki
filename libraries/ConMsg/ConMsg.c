#include "ConMsg.h"




#define	Z_SET_INT16(p,val) \
			(((uint8_t *) p) [0] = ((val)>>0) & 0xff, \
			 ((uint8_t *) p) [1] = ((val)>>8) & 0xff )
#define	Z_GET_INT16(p)	( (((uint8_t *) p) [0] << 0) | \
			  (((uint8_t *) p) [1] << 8) )

#define I154_ADDRLEN 2


ConMsg *conmsg ;


ConStat *getstat () { return &conmsg->stat_ ; }

int getMsgbufsize () { return conmsg->msgbufsize_ ; }

addr2_t getAddr2 () { return conmsg->addr2_ ; }

addr8_t getAddr8 () { return conmsg->addr8_ ; }

panid_t getPanid () { return conmsg->panid_ ; }

channel_t getChannel () { return conmsg->chan_ ; }

void setMsgbufsize ( int msgbufsize) { conmsg->msgbufsize_ = msgbufsize ; }

void setAddr2 ( addr2_t addr) { conmsg->addr2_ = addr ; }

void setAddr8 ( addr8_t addr) { conmsg->addr8_ = addr ; }

void setPanid ( panid_t panid) { conmsg->panid_ = panid ; }

void setChannel ( channel_t chan) {  conmsg->chan_ = chan ; }


uint8_t *usr_radio_receive_frame (uint8_t len, uint8_t *frm) {
	return it_receive_frame( len, frm);
}

uint8_t *it_receive_frame ( uint8_t len, uint8_t *frm)
{
	//printf ("reçu\n");
	int newlast = (conmsg->rbuflast_ + 1) % conmsg->msgbufsize_ ;

	if (newlast == conmsg->rbuffirst_)
	    conmsg->stat_.rx_overrun++ ;
	else
	{
	    /*
	     * There is room for a new frame:
	     * - update the message with length and lqi
	     * - update the frame buffer pointer
	     */
	    conmsg->rbuffer_ [conmsg->rbuflast_].len = len ;

	    conmsg->rbuflast_ = newlast ;

	    frm = (uint8_t *) conmsg->rbuffer_ [newlast].frame ;
	}
	//printf("%d   :   %d\n", conmsg->rbuffirst_, conmsg->rbuflast_);
    return frm;
}


/*
 * Called by interrupt routine (see radio_rfa.c) when a transmission
 * is done. Update statistics.
 */

void usr_radio_tx_done ()
{
	it_tx_done();
}


void it_tx_done ()
{
	
    conmsg->writing_ = false ;
}



void init() {
	conmsg->chan_ = 17;
	conmsg->writing_ = false;
}


void start() {
	
	if (conmsg->msgbufsize_ == 0)		// prevent stupid errors...
		conmsg->msgbufsize_ = DEFAULT_MSGBUF_SIZE ;

	if (conmsg->rbuffer_ != NULL) {
		conmsg->rbuffer_ =NULL ;	
	}
	
    conmsg->rbuffer_ = (ConBuf *)malloc(sizeof(struct ConBuf)*conmsg->msgbufsize_) ;

    conmsg->rbuffirst_ = 0 ;
    conmsg->rbuflast_ = 0 ;
    
    conmsg->writing_ = false;
    conmsg->seqnum_ = 0;

    NETSTACK_RADIO.init();
    initBuf((uint8_t *) conmsg->rbuffer_ [conmsg->rbuflast_].frame, MAX_PAYLOAD);
    NETSTACK_RADIO.on();

}



bool sendto (  addr2_t a,  const uint8_t payload[], uint8_t len ) {
	uint8_t frame[MAX_PAYLOAD];
	uint16_t fcf ;
	int frmlen ;
	frmlen = 9 + len ;
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

	Z_SET_INT16 (&frame [0], fcf) ;		// fcf
    frame [2] = ++conmsg->seqnum_ ; ;			// seq
    Z_SET_INT16 (&frame [3], conmsg->panid_) ;		// dst panid
    Z_SET_INT16 (&frame [5], a) ;		// dst addr
    Z_SET_INT16 (&frame [7], conmsg->addr2_) ;		// src addr

    memcpy (frame + 9, payload, len) ;

    // int i;
    // for (i = 0; i < frmlen; ++i)
    // {
    // 	printf("%d",frame[i] );
    // }
    // printf("\n");
    //printf("envoyé\n" );
    conmsg->writing_ = true ;
    NETSTACK_RADIO.send (frame, frmlen) ;

    while (conmsg->writing_);
	return true;
}


ConReceivedFrame *get_received () {
	ConReceivedFrame *r ;
    ConBuf *b ;

    platform_enter_critical();
    if (conmsg->rbuffirst_ == conmsg->rbuflast_) {
		b = NULL ;
	}
    else  b = &conmsg->rbuffer_ [conmsg->rbuffirst_] ;
    platform_exit_critical();
    if (b == NULL){
		r = NULL ;
	}
    else
    {

    	uint8_t *p = (uint8_t *) b->frame ;
		int intrapan ;

		r = & conmsg->rframe_ ;
		r->rawframe = (uint8_t *) b->frame ;
		r->rawlen = b->len ;
		r->lqi = b->lqi;

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
		r->paylen = r->rawlen - (p - r->rawframe)  ;	// skip FCS

    }
    return r ;

}



void skip_received ()
{

    platform_enter_critical();
    if (conmsg->rbuffirst_ != conmsg->rbuflast_)
		conmsg->rbuffirst_ = (conmsg->rbuffirst_ + 1) % conmsg->msgbufsize_ ;
    platform_exit_critical();
}

