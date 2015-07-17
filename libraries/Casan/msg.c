/**
 * @file msg.cpp
 * @brief Msg class implementation
 */

#include "msg.h"

// CASAN is based on this CoAP version
#define	CASAN_VERSION	1

#define	FORMAT_BYTE0(ver,type,toklen)				\
			((((unsigned int) (ver) & 0x3) << 6) |	\
			 (((unsigned int) (type) & 0x3) << 4) |	\
			 (((unsigned int) (toklen) & 0x7))	\
			 )
#define	COAP_VERSION(b)	(((b) [0] >> 6) & 0x3)
#define	COAP_TYPE(b)	(((b) [0] >> 4) & 0x3)
#define	COAP_TOKLEN(b)	(((b) [0]     ) & 0xf)
#define	COAP_CODE(b)	(((b) [1]))
#define	COAP_ID(b)	(((b) [2] << 8) | (b) [3])

#define	OPTVAL(o)	((o)->optval_ ? (o)->optval_ : (o)->staticval_)


/******************************************************************************
Constructor, destructor, operators
******************************************************************************/

/* free Msg */
void freeMsg(Msg *m){
	free(m->payload_);
	free(m->encoded_);
	free(m);
}


/**
 * Default constructor: initialize an empty message.
 *
 * The current L2 network is associated to a message.
 *
 * @param l2 pointer to the l2 network associated to this message
 */

Msg *initMsg(l2net_154 *l2) {
	Msg *m = (Msg *)malloc (sizeof( Msg));

	m->l2_ = l2;
	m->paylen_ = 0;
	m->type_ = 0;
	m->id_ = 0;
	m->code_ = 0;
	m->payload_ = NULL;
	m->optlist_ = NULL;
	m->encoded_ = NULL;
	m->token_= initToken();
	return m;
}



/**
 * Copy-constructor: copy a message, including its payload and its
 * options.
 */

Msg *initMsgMsg (const Msg *m2) 
{
	Msg *m = (Msg *)malloc (sizeof(struct msg));
    msgcopy (m, m2) ;
    return m;
}

/**
 * Destructor
 */

void initMsgDes (Msg *m)
{
    resetMsg (m) ;
}


/**
 * Reset function: free memory, etc.
 */
void resetMsg(Msg *m) {
	l2net_154 *l2;

	l2 = m->l2_;
	if(m->payload_ != NULL)
		free (m->payload_);
	if (m->encoded_ != NULL)
		free (m->encoded_);
	while (m->optlist_ != NULL)
		freeOption(pop_option(m));
	m->l2_ = l2;
}


void copyMsg(Msg *m1, const Msg *m2) {
	if (!(isEqualMsg(m1, m2)))
		msgcopy(m1, m2);
}


bool isEqualMsg(const Msg *m1, const Msg *m2) {
	return m1->id_ == m2->id_;
}




// accessors (for received messages)
uint8_t  get_type    (Msg *m)	{ return m->type_ ; }
uint8_t  get_code    (Msg *m)	{ return m->code_ ; }
uint16_t get_id      (Msg *m)	{ return m->id_ ; }
token   *get_token_msg   (Msg *m)	{ return m->token_ ; }
uint16_t get_paylen_msg  (Msg *m)	{ return m->paylen_ ; }
uint8_t *get_payload_msg (Msg *m)	{ return m->payload_ ; }



// mutators (to send messages)
void set_type    (Msg *m, uint8_t t)	{ m->type_ = t ; }
void set_code    (Msg *m, uint8_t c)	{ m->code_ = c ; }
void set_id      (Msg *m, uint16_t id)	{ m->id_ = id ; }
void set_token_msg   (Msg *m, token *tok)	{ m->token_ = tok ; }



/******************************************************************************
 * Receive message
 *
 * Methods for:
 * - receive and decode a message
 * - access informations from the received message
 */

/**
 * @brief Receive and decode a message
 *
 * Receive a message on a given L2 network, and decode it
 * according to CoAP specification if it is a valid incoming
 * message (`l2net::RECV_OK`) or if it has been truncated
 * (`l2net::RECV_TRUNCATED`). In this case, only the first part
 * of the message (excluding options and payload) is decoded.
 * This method may return `l2net::RECV_EMPTY` if no message has been
 * received, or any other value returned by the L2net-* `recv`
 * methods (such as `l2net::RECV_WRONG_TYPE` in the case of
 * Ethernet).
 *
 * @return receive status (see l2net class)
 */


l2_recv_t recvMsg (Msg *m) {
	l2_recv_t r;

	resetMsg(m);
	r = recv(m->l2_) ;
	
	if (r == RECV_OK || r == RECV_TRUNCATED) {
		bool trunc = (r == RECV_TRUNCATED) ;
		
		if (! coap_decode (m, get_payload (m->l2_,0), get_paylen (m->l2_), trunc))
	    	r = RECV_EMPTY ;
	    // printMsg(m);
	}
	
	return r;
}



/**
 * @brief Decode a message according to CoAP specification.
 *
 * Returns true if message decoding was successful.
 * If message has been truncated, decoding is done only for
 * CoAP header and token (and considered as a success).
 *
 * @param rbuf	L2 payload as received by the L2 network
 * @param len	Length of L2 payload
 * @param truncated true if the message has been truncated at reception
 * @return True if decoding was successful (even if truncated)
 */

bool coap_decode (Msg *m, uint8_t rbuf [], size_t len, bool truncated)
{
	bool success ;

	resetMsg(m);
	success = true;

	if (COAP_VERSION (rbuf) != CASAN_VERSION)
    {
    	success = false;
    } else {
    	size_t i ;
		int opt_nb ;

		m->type_ = COAP_TYPE (rbuf) ;
		m->token_->toklen_ = COAP_TOKLEN (rbuf) ;
		m->code_ = COAP_CODE (rbuf) ;
		m->id_ = COAP_ID (rbuf) ;
		i = 4 ;

		if (m->token_->toklen_ > 0) {
			memcpy (m->token_->token_, rbuf + i, m->token_->toklen_) ;
			i += m->token_->toklen_ ;
		}

		/*
		 * Options analysis
		 */

		opt_nb = 0 ;
		
		while (! truncated && success && i < len && rbuf [i] != 0xff)
		{	//printf("%lu\n",rbuf [i]  );
			int opt_delta = 0;
			int opt_len = 0 ;
			option *o = initOption();

			opt_delta = (rbuf [i] >> 4) & 0x0f ;
		    opt_len   = (rbuf [i]     ) & 0x0f ;
		    i++ ;
		    switch (opt_delta)
		    {
			case 13 :
				opt_delta = rbuf [i] + 13 ;
			    i += 1 ;
			    break ;
			case 14 :
			    opt_delta = (rbuf [i] << 8) + rbuf [i+1] + 269 ;
			    i += 2 ;
			    break ;
			case 15 :
			    success = false ;			// recv failed
			    break ;
			}
			opt_nb += opt_delta ;

			switch (opt_len)
		    {
			case 13 :
			    opt_len = rbuf [i] + 13 ;
			    i += 1 ;
			    break ;
			case 14 :
			    opt_len = (rbuf [i] << 8) + rbuf [i+1] + 269 ;
			    i += 2 ;
			    break ;
			case 15 :
			    success = false ;			// recv failed
			    break ;
		    }

		    /* register option */
		    if (success)
		    {	
				setOptcode (o,(optcode_t)opt_nb) ;
				setOptvalOpaque (o, (void *)(rbuf + i), (int) opt_len) ;
				push_option (m, o) ;
				
				i += opt_len ;
		    }
		    else
		    {
				success = false ;
				printf  ("%s",RED ("Option unrecognized")) ;
				printf (" opt_delta = %d", opt_delta) ;
				printf (" opt_len = %d", opt_len) ;
				printf ("\n");
		    }
		    
		    
		}
		

		m->paylen_ = len - i ;
		
		if (! truncated && success && m->paylen_ > 0) {
			if (rbuf [i] != 0xff)
		    {
				success = false ;
		    }
		    else
		    {
				i++ ;
				set_payload_msg (m, rbuf + i, m->paylen_) ;

		    }
		} else m->paylen_ = 0 ;			// protect further operations

    }

    return success;

}


/******************************************************************************
 * Send message
 *
 * Methods for:
 * - encode a message
 * - access informations from the received message
 */

/**
 * @brief Encode and send a message
 *
 * Encode a message according to CoAP specification and
 * send the result to the given L2 address on the given
 * L2 network.
 *
 * Memory is allocated for the encoded message. It will
 * be freed when the object will be destroyed (the encoded
 * message is kept since it may have to be retransmitted).
 * If the encoded message does not fit in this buffer, this
 * method reports an error (false value)
 *
 * Note: the return value is the value returned by the `send`
 * method of the appropriate L2net-* class. It means that
 * the message has been sent to the network hardware, and
 * does not mean that the message has been successfully sent.
 *
 * @param dest L2 destination address
 * @return true if encoding was successfull
 */

bool sendMsg (Msg *m, l2addr_154 *dest) 
{
	int success ;
	if (m->encoded_ == NULL)
    {
    	m->enclen_ = maxpayload (m->l2_) ;// exploitable size
		m->encoded_ = (uint8_t *) malloc (m->enclen_) ;
		success = coap_encode (m, m->encoded_, &m->enclen_) ;
		if (! success)
	   		printf ("%s",RED ("Cannot encode the message\n")) ;
	} else success = true ;			// if msg is already encoded

	if (success)
    {	
		success = send (m->l2_, dest, m->encoded_, m->enclen_) ;
		if (! success)
		    printf ("%s",RED ("Cannot L2-send the message\n")) ;	
    } else {
    	free (m->encoded_) ;
		m->encoded_ = NULL ;
    }
    return success;
}



/**
 * @brief Encode a message according to the CoAP specification
 *
 * If message does not fit in the given buffer, an error is returned.
 *
 * @param sbuf memory allocated for the encoded message
 * @param sbuflen size of sbuf
 * @return true if encoding was successfull
 */

bool coap_encode (Msg *m, uint8_t sbuf [], uint16_t *sbuflen)
{
	uint16_t i ;
    uint16_t opt_nb ;
    uint16_t size ;
    bool success ;
    bool emulpayload ;

    /*
     * Format message, part 1 : compute message size
     */

    emulpayload = false ;		// get the true available space
    size = coap_size (m, emulpayload) ;

    if (size <= *sbuflen)		// Enough space?
    {
		success = true ;
		*sbuflen = size ;
		/*
		 * Format message, part 2 : build message
		 */
		 i = 0 ;

		// header
		sbuf [i++] = FORMAT_BYTE0 (CASAN_VERSION, m->type_, m->token_->toklen_) ;
		sbuf [i++] = m->code_ ;
		sbuf [i++] = BYTE_HIGH (m->id_) ;
		sbuf [i++] = BYTE_LOW  (m->id_) ;
		
	printf("\n");
		// token
		if (m->token_->toklen_ > 0)
		{
		    memcpy (sbuf + i, m->token_->token_, m->token_->toklen_) ;
		    i += m->token_->toklen_ ;
		}
		// options
		reset_next_option (m) ;
		opt_nb = 0 ;
		option *o;
		for ( o = next_option (m) ; o != NULL ; o = next_option (m))
		{
			int opt_delta, opt_len ;
	    	int posoptheader = i ;

	    	sbuf [posoptheader] = 0 ;

	    	i++ ;
	    	opt_delta = (int)o->optcode_ - opt_nb ;

	    	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	    	{
	    		opt_delta -= 269 ;
				sbuf [i++] = BYTE_HIGH (opt_delta) ;
				sbuf [i++] = BYTE_LOW  (opt_delta) ;
				sbuf [posoptheader] |= 0xe0 ;
			} 
			else if (opt_delta >= 13)		// delta \in [13..268] => 1 byte
	    	{
	    		opt_delta -= 13 ;
				sbuf [i++] = BYTE_LOW (opt_delta) ;
				sbuf [posoptheader] |= 0xd0 ;
			}
			else
		    {
				sbuf [posoptheader] |= (opt_delta << 4) ;
		    }
		    opt_nb = o->optcode_ ;

		    opt_len = o->optlen_ ;
		    if (opt_len >= 269)			// len >= 269 => 2 bytes
		    {
				opt_len -= 269 ;
				sbuf [i++] = BYTE_HIGH (opt_len) ;
				sbuf [i++] = BYTE_LOW  (opt_len) ;
				sbuf [posoptheader] |= 0x0e ;
		    }
		    else if (opt_len >= 13)		// len \in [13..268] => 1 byte
		    {
				sbuf [i++] = BYTE_LOW (opt_len) ;
				sbuf [posoptheader] |= 0x0d ;
		    }
		    else
		    {
				sbuf [posoptheader] |= opt_len ;
		    }

	
		    memcpy (sbuf + i, OPTVAL (o), o->optlen_) ;
	    	i += o->optlen_ ;
		}

		// payload
		if (m->paylen_ > 0)
		{
		    sbuf [i++] = 0xff ;			// start of payload
		    memcpy (sbuf + i, m->payload_, m->paylen_) ;
		}
	} 
	else
    {
		printf ("Message truncated on CoAP encoding") ;
		success = false ;
    }

    return success ;
}


// /**
//  * @brief Compute encoded message size
//  * 
//  * Compute the size of the message when it will be encoded according
//  * to the CoAP specification. This computation is done according to
//  * token, options and payload currently associated with the message.
//  * Since the end of options is marked with a 0xff byte before the
//  * payload, we have to know if a payload will be added in the future
//  * in order to estimate available space in the message.
//  *
//  * @param emulpayload true if a payload will be added in the future
//  * @return estimated size of the encoded message
//  */

size_t coap_size (Msg *m, bool emulpayload)
{
    uint16_t opt_nb ;
    size_t size ;

    size = 4 + m->token_->toklen_ ;

    reset_next_option (m) ;
    opt_nb = 0 ;
    option *o;
    for ( o = next_option (m) ; o != NULL ; o = next_option (m))
    {
		int opt_delta, opt_len ;

		size++ ;			// 1 byte for opt delta & len

		opt_delta = o->optcode_ - opt_nb ;
		if (opt_delta >= 269)		// delta >= 269 => 2 bytes
		    size += 2 ;
		else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
		    size += 1 ;
		opt_nb = o->optcode_ ;

		opt_len = o->optlen_ ;
		if (opt_len >= 269)		// len >= 269 => 2 bytes
		    size += 2 ;
		else if (opt_len >= 13)		// len \in [13..268] => 1 byte
		    size += 1 ;
		size += o->optlen_ ;

    }
    
    if (m->paylen_ > 0 || emulpayload)
		size += 1 + m->paylen_ ;		// don't forget 0xff byte
    return size ;
}



/**
 * @brief Estimate the available space in the message
 *
 * Compute the available space in the message, according to L2 MTU
 * and size of message when it will be encoded. Typically used to
 * know available space for the payload.
 *
 * @return Available space in the message, or 0 if the message does
 * not fit.
 */

size_t avail_space (Msg *m)
{	
    size_t size, maxpayld, avail ;

    size = coap_size (m, true) ;
    maxpayld = maxpayload (m->l2_) ; 

    avail = (size <= maxpayld) ? maxpayld - size : 0 ;
    return avail ;
}


/*
 * Various mutators to long to be inlined: token and payload
 */

void set_payload_msg (Msg *m, uint8_t *payload, uint16_t paylen) 
{
    m->paylen_ = paylen ;
    if (m->payload_ != NULL)
		free (m->payload_) ;
    m->payload_ = (uint8_t *) malloc (m->paylen_) ;
    
    memcpy (m->payload_, payload, m->paylen_) ;

}




// /******************************************************************************
//  * Option management
//  */

// /**
//  * @brief Remove the first option from the option list
//  *
//  * @return First option
//  */

option *pop_option (Msg *m) 
{
	option *r = NULL;
	if( m->optlist_) {
		optlist *next;

		r = m->optlist_->o;
		next = m->optlist_->next;
		free(m->optlist_);
		m->optlist_ = next;
	}
	return r;
}


/**
 * @brief Push an option in the option list
 *
 * The option list is kept sorted according to option values
 * in order to optimally encode CoAP options.
 */

void push_option (Msg *m, option *o) 
{

    optlist *newo, *prev, *cur ;

    newo = (optlist *) malloc (sizeof (struct optlist));
    newo->o = initOptionOption(o);

    prev = NULL ;
    cur = m->optlist_ ;

    while (cur != NULL && isGreaterOrEqual(newo->o, cur->o))
    {
		prev = cur ;
		cur = cur->next ;
    }
    newo->next = cur ;
    if (prev == NULL)
		m->optlist_ = newo ;
    else
		prev->next = newo ;

}


/**
 * @brief Reset the option iterator
 *
 * This method resets the internal pointer (in option list) used by the
 * the option iterator (see `next_option`).
 */

void reset_next_option (Msg *m) 
{
    m->curopt_initialized_ = false ;
}



/**
 * @brief Option iterator
 *
 * Each call to this method will return the next element in option
 * list, thanks to an internal pointer which is advanced in this method.
 * Looping through options must start by a call to
 * `reset_next_option' before first use.
 */

option *next_option (Msg *m) 
{
    option *o ;
    if (! m->curopt_initialized_)
    {	
		m->curopt_ = m->optlist_ ;
		m->curopt_initialized_ = true ;
    }
    if (m->curopt_ == NULL)
    {	
		o = NULL ;
		m->curopt_initialized_ = false ;
    }
    else
    {	
		o = m->curopt_->o ;
		m->curopt_ = m->curopt_->next ;
    }
    return o ;
}


/**
 * @brief Search for a specific (and unique) option
 *
 * This method returns the first option in the option list which
 * match the given opcode. If not found, returns `NULL`.
 */

option *search_option (Msg *m, optcode_t c)
{
    optlist *ol ;
    option *o ;

    o = NULL ;
    for (ol = m->optlist_ ; ol != NULL ; ol = ol->next)
    {
		if (ol->o->optcode_ == c)
		{
		    o = ol->o ;
		    break ;
		}
    }
    return o ;
}


/*
 * Copy a whole message, including payload and option list
 */

void msgcopy (Msg *m1, const Msg *m2) {
	optlist *ol1, *ol2 ;

	memcpy(m1, &m2, sizeof m2);

	if (m1->payload_)
		free(m1 -> payload_);

	m1->payload_ = (uint8_t *) malloc (m1->paylen_) ;
	memcpy (m1->payload_, m2->payload_, m1->paylen_);

	m1->enclen_ = 0;
	if (m1->encoded_ != NULL) 
		free(m1->encoded_);
	m1->encoded_ = NULL;

	m1->optlist_ = NULL;
	m1->curopt_ = NULL;
	m1->curopt_initialized_ = false;

	ol1 = NULL;
	for (ol2 = m2->optlist_; ol2 != NULL ; ol2 = ol2->next) {
		optlist *newo;
		newo = (optlist *) malloc (sizeof (struct optlist));
		newo->o = initOptionOption(ol2->o);
		newo->next = NULL;
		if (ol1 == NULL)
			m1->optlist_ = newo;
		else
			ol1->next = newo;
		ol1 = newo;
	}

}


/**
 * @brief Returns content_format option
 *
 * This method returns the content_format option, if present,
 * or `option::cf_none` if not present.
 *
 * @return value associated with the content_format option or option::cf_none
 */

content_format get_content_format (Msg *m)
{
    optlist *ol ;
    content_format cf ;
    
    cf = cf_none ;		// not found by default ;
    for (ol = m->optlist_ ; ol != NULL ; ol = ol->next)
    {
		if (getOptCode (ol->o) == MO_Content_Format)
		{
		    cf = (content_format) getOptvalInteger (ol->o) ;
		    break ;
		}
    }
    return cf ;
} 



/**
 * @brief Set content_format option
 *
 * If the content_format option is not already present in option list,
 * this method adds the option.
 * If the reset argument is true, the content_format option value
 * is reset to the given value (otherwise, option is not modified).
 *
 * @param reset true if the existing option, if exists, must be reset
 *	to the given value
 * @param cf the new value
 */

void set_content_format (Msg *m, bool reset, content_format cf)
{
    optlist *ol ;

    // look for the ContentFormat option
    for (ol = m->optlist_ ; ol != NULL ; ol = ol->next)
		if (getOptcode (ol->o) == MO_Content_Format)
		    break ;

    if (ol != NULL)			// found
    {
		if (reset)			// reset it to the new value?
		    setOptvalInteger (ol->o, cf) ;	// yes
    }
    else				// not found: add this option
    {
		option *ocf ;

		ocf = initOptionInteger (MO_Content_Format, cf_text_plain) ;
		push_option (m, ocf) ;
		freeOption(ocf) ;
    }
}



/**
 * @brief Returns Max-Age option
 *
 * This method returns the Max-Age option, if present,
 * or 0 if not present.
 *
 * @return value associated with the Max-Age option
 */

time_t get_max_age (Msg *m)
{
    optlist *ol ;
    time_t t ;
    
    t = 0 ;				// not found by default ;
    for (ol = m->optlist_ ; ol != NULL ; ol = ol->next)
    {
		if (getOptcode (ol->o) == MO_Max_Age)
		{
		    t = getOptvalInteger (ol->o) ;
		    break ;
		}
    }
    return t ;
}



/**
 * @brief Set Max-Age option
 *
 * If the Max-Age option is not already present in option list,
 * this method adds the option.
 * If the reset argument is true, the content_format option value
 * is reset to the given value (otherwise, option is not modified).
 *
 * @param reset true if the existing option, if exists, must be reset
 *	to the given value
 * @param cf the new value
 */

void set_max_age (Msg *m, bool reset, time_t dur)
{
    optlist *ol ;

    // look for the Max-Age option
    for (ol = m->optlist_ ; ol != NULL ; ol = ol->next)
	if (getOptcode (ol->o) == MO_Max_Age)
	    break ;

    if (ol != NULL)			// found
    {
		if (reset)			// reset it to the new value?
		    setOptvalInteger  (ol->o, (long int) dur) ;	// yes
    }
    else				// not found: add this option
    {
		option *ocf ;

		ocf = initOptionInteger (MO_Max_Age, (long int) dur) ;
		push_option (m, ocf) ;
		free(ocf) ;
    }
}


/*
 * Useful for debugging purposes
 */

void printMsg (Msg *m) 
{
    char *p ;
    int len ;
    int i ;

    printf("%s id = %d",BLUE ("msg"), get_id(m) ) ;
    printf(", type = ") ;
    switch (get_type (m)) {
		case COAP_TYPE_CON : printf ("CON") ; break ;
		case COAP_TYPE_NON : printf ("NON") ; break ;
		case COAP_TYPE_ACK : printf ("ACK") ; break ;
		case COAP_TYPE_RST : printf ("RST") ; break ;
		default : printf("%s",RED ("ERROR")) ;
    }
    printf (", code = %lu", get_code(m) >> 5) ;
    printf (".") ;
    printf ("%lu", get_code (m) & 0x1f) ;
    printf (", toklen = %d", m->token_->toklen_) ;

    if (m->token_->toklen_ > 0) {
		printf (", token = ") ;
		printToken (m->token_) ;
		printf("\n");
    }

    p = (char *) get_payload_msg (m) ;
    len = get_paylen_msg (m) ;
    printf (", paylen = %d", len) ;
    printf (", payload = ") ;
    for (i = 0 ; i < len ; i++)
		printf ("%c", p [i]) ;
    printf("\n"); 
    

    reset_next_option (m) ;
    option *o;
    for (o = next_option (m) ; o != NULL ; o = next_option (m)) {
		printOption (o) ;
    }
}