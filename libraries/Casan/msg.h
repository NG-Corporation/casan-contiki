/**
 * @file msg.h
 * @brief Msg class interface
 */

#ifndef __MSG_H__
#define __MSG_H__

#include "../L2-154/l2-154.h"
#include "option.h"
#include "token.h"
#include "stdbool.h"
#include "time.h"

#define COAP_RETURN_CODE(x,y) ((x << 5) | (y & 0x1f))

// the offset to get pieces of information in the MAC payload
#define	COAP_OFFSET_TYPE	0
#define	COAP_OFFSET_TKL		0
#define	COAP_OFFSET_CODE	1
#define	COAP_OFFSET_ID		2
#define	COAP_OFFSET_TOKEN	4


/** CoAP methods */
typedef enum coap_code {
    COAP_CODE_EMPTY = 0,
    COAP_CODE_GET,
    COAP_CODE_POST,
    COAP_CODE_PUT,
    COAP_CODE_DELETE
} coap_code_t;

/** CoAP message types */
typedef enum coap_type {
    COAP_TYPE_CON = 0,
    COAP_TYPE_NON,
    COAP_TYPE_ACK,
    COAP_TYPE_RST
} coap_type_t;



/**
 * @brief An object of class Msg represents a message,
 * either received or to be sent
 *
 * This class represents messages to be sent to the network, or
 * received from the network.
 *
 * Message attributes are tied to CoAP specification: a message has
 * a type (CON, NON, ACK, RST), a code (GET, POST, PUT, DELETE, or a numeric
 * value for an answer in an ACK), an Id, a token, some options and
 * a payload.
 *
 * In order to be sent to the network, a message is transparently
 * encoded (by the `send` method) according to CoAP specification.
 * Similarly, a message is transparently decoded (by the `recv`
 * method) upon reception according to the CoAP specification.
 *
 * A received message is stored in the appropriate L2 receive buffer
 * (see the various L2net-* classes). This buffer is allocated at
 * the program startup and never freed. As such, there can be at most
 * one received message.
 */


 	typedef struct optlist
	{
	    option *o ;
	    struct optlist *next ;
	} optlist;


	typedef struct msg {
		l2net_154   *l2_ ;
		uint8_t *encoded_ ;	// encoded message to send
		uint16_t enclen_ ;	// real size of msg (encoded_ may be larger)
		
		uint8_t  type_ ;
		uint8_t  code_ ;
		uint16_t id_ ;
		token    *token_ ;
		uint16_t paylen_ ;
		uint8_t *payload_ ;
		uint8_t  optlen_ ;
		optlist *optlist_ ;		// sorted list of all options
		optlist *curopt_ ;		// current option (position in opt list)
		bool     curopt_initialized_ ;	// is curopt_ initialized ?
	} Msg;



	void freeMsg(Msg *m);

	Msg *initMsg(l2net_154 *l2);
	Msg *initMsgMsg (const Msg *m2);
	void initMsgDes (Msg *m);

	void resetMsg(Msg *m);

	void copyMsg(Msg *m1, const Msg *m2);

	bool isEqualMsg(const Msg *m1, const Msg *m2);

	uint8_t  get_type    (Msg *m);
	uint8_t  get_code    (Msg *m);
	uint16_t get_id      (Msg *m);
	token   *get_token_msg   (Msg *m);
	uint16_t get_paylen_msg  (Msg *m);
	uint8_t *get_payload_msg (Msg *m);

	void set_type    (Msg *m, uint8_t t);
	void set_code    (Msg *m, uint8_t c);
	void set_id      (Msg *m, uint16_t id);
	void set_token_msg   (Msg *m, token *tok);
	void set_payload_msg (Msg *m, uint8_t *payload, uint16_t paylen) ;

	l2_recv_t recvMsg (Msg *m);

	bool coap_decode (Msg *m, uint8_t rbuf [], size_t len, bool truncated);

	bool sendMsg (Msg *m, l2addr_154 *dest);

	bool coap_encode (Msg *m, uint8_t sbuf [], uint16_t *sbuflen);

	size_t coap_size (Msg *m, bool emulpayload);
	size_t avail_space (Msg *m);
	
	option *pop_option (Msg *m);
	void push_option (Msg *m, option *o);

	void reset_next_option (Msg *m);
	option *next_option (Msg *m);
	option *search_option (Msg *m, optcode_t c);

	void msgcopy (Msg *m1, const Msg *m2);

	content_format get_content_format (Msg *m);
	void set_content_format (Msg *m, bool reset, content_format cf);

	time_t get_max_age (Msg *m);
	void set_max_age (Msg *m, bool reset, time_t dur);

	void printMsg (Msg *m);



#endif