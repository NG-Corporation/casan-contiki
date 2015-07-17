/**
 * @file resource.h
 * @brief Resource class interface
 */

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "msg.h"

/**
 * @brief An object of class Resource represents a resource which
 *	is provided by the application and managed by the CASAN engine.
 *
 * This class represents a resource. A resource has:
 * - some attributes: name, title, etc.
 * - a handler for each CoAP operation (GET, PUT, etc.)
 * - a textual representation for `/.well-known/casan` aggregation
 * - observe information
 *
 * The handler prototype (see the `handler_t` typedef) is:
 *	`uint8_t handler (Msg *in, Msg *out)`
 * The message `out` is prepared by the CASAN engine with some items
 * from the incoming message (ACK, id, token) before calling the handler.
 * Rest of message must be provided by the handler, except code which
 * will be filled with the return value of the handler.
 * Note that the handler may provide the content-format option if
 * `text_plain` is not the wanted default.
 * Note that the handler is called with in == NULL if the message
 * to be sent is due to an observation trigger.
 *
 * The observe information is set by the `ohandler` method, which
 * takes 3 parameters:
 * - a handler called when a observe message is received
 * - a handler called when a deregistering event is detected
 * - a handler called to check if the observed event is detected
 *   and a message is to be sent (the message will be sent by the
 *   message handler registered with the `handler` method)
 */

	/** Handler prototype. See class description for details.
	 */
	typedef uint8_t (*handler_res_t) (Msg *in, Msg *out) ;

	/**
	 * Register observation prototype.
	 */

	typedef void (*obs_register_t) (Msg *in) ;

	/**
	 * Unregister observation prototype.
	 */

	typedef void (*obs_deregister_t) (void) ;

	/**
	 * Observation trigger prototype (returns true if an observe
	 * message must be sent)
	 */

	typedef int (*obs_trigger_t) (void) ;



	typedef struct resource {
		handler_res_t handler_ [5] ;		// indexed by coap_code_t

		char *name_ ;
		char *title_ ;
		char *rt_ ;

		bool observed_ ;			// resource currently observed
		obs_register_t obs_reg_ ;		// register an observer
		obs_deregister_t obs_dereg_ ;		// unregister an observer
		obs_trigger_t obs_trig_ ;		// detect observe event
		uint32_t obs_serial_ ;			// increasing value for option
		token *obs_token_ ;
	} Resource;


	Resource *initResource (const char *name, const char *title, const char *rt);

	/** Accessor function
	 *
	 * @return name of resource (do not free this string)
	 */
	void freeResource (Resource *rs) ;

	char *get_name (Resource *rs)	;

	void setHandlerResource (Resource *rs, coap_code_t op, handler_res_t h);

	handler_res_t getHandlerResource (Resource *rs, coap_code_t op);

	void ohandlerResource (Resource *rs, obs_register_t reg, obs_deregister_t dereg, obs_trigger_t trig);

	void observedResource (Resource *rs, bool onoff, Msg *m);
	bool get_observed (Resource *rs) ;

	int check_trigger (Resource *rs);
	uint32_t next_serial (Resource *rs) ;
	token *get_token (Resource *rs)	;

	int well_known (Resource *rs , char *buf, size_t maxlen);

	void printResource (Resource *rs);



#endif