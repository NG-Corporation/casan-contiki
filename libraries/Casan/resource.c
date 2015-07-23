/**
 * @file resource.cpp
 * @brief Resource class implementation
 */

#include "resource.h"

#define	ALLOC_COPY(d,s)		do {				\
				    (d) = (char *) malloc (strlen (s) + 1) ; \
				    strcpy ((d), (s)) ;		\
				} while (false)			// no ";"


char *get_name (Resource *rs)       { return rs->name_ ; }
bool get_observed (Resource *rs)        { return rs->observed_ ; }
uint32_t next_serial (Resource *rs)     { return ++rs->obs_serial_ ; }
token *get_token (Resource *rs)     { return rs->obs_token_ ; }

/** @brief Copy constructor
 */
Resource *initResource (const char *name, const char *title, const char *rt)
{
    int i;
	Resource *rs = (Resource *) malloc (sizeof (Resource));
    ALLOC_COPY (rs->name_, name) ;
    ALLOC_COPY (rs->title_, title) ;
    ALLOC_COPY (rs->rt_, rt) ;
    for ( i = 0 ; i < NTAB (rs->handler_) ; i++)
	   rs->handler_ [i] = NULL ;
    rs->observed_ = false ;
    rs->obs_trig_ = NULL ;
    rs->obs_reg_ = NULL ;
    rs->obs_dereg_ = NULL ;
    return rs;
}



/** @brief Destructor
 */

void freeResource (Resource *rs) {
	free(rs->name_);
	free(rs->title_);
	free(rs->rt_);
	free(rs);
}



/** @brief Set resource handler
 *
 * @param op CoAP operation (see coap_code_t type)
 * @param h Function to handle request and provide a response
 */

void setHandlerResource (Resource *rs, coap_code_t op, handler_res_t h)
{
    rs->handler_ [op] = h ;
}


/** @brief Get resource handler
 *
 * @param op CoAP operation (see coap_code_t type)
 * @return Handler for this operation
 */

handler_res_t getHandlerResource (Resource *rs, coap_code_t op)
{
    return rs->handler_ [op] ;
}


/** @brief Set observe handlers
 *
 * @param reg handler for registering the observation (may be null)
 * @param dereg handler for deregistering the observation (may be null)
 * @param trigger handler for detecting a change (must not be null)
 */

void ohandlerResource (Resource *rs, obs_register_t reg, obs_deregister_t dereg, obs_trigger_t trig)
{
    rs->observed_ = false ;
    rs->obs_reg_ = reg ;
    rs->obs_dereg_ = dereg ;
    rs->obs_trig_ = trig ;
    rs->obs_serial_ = 0 ;
}



/** @brief Register or deregister an observer
 *
 * @param onoff true (register) or false (deregister)
 * @param m incoming message (for registering) or NULL (for deregistering)
 */

void observedResource (Resource *rs, bool onoff, Msg *m)
{
    if (rs->obs_trig_ != NULL)
    {
		if (rs->observed_ && rs->obs_dereg_ != NULL)
		    (*rs->obs_dereg_) () ;

		rs->observed_ = onoff ;
		if (onoff)
		{
		    if (rs->obs_reg_ != NULL)
			(*rs->obs_reg_) (m) ;
		    rs->obs_serial_ = 2 ;			/* starting value */
		    rs->obs_token_ = get_token_msg (m) ;
		}
    }
}


/** @brief Detect observe events
 *
 * @return 1 if an observe message must be sent
 */

int check_trigger (Resource *rs)
{
    return rs->obs_trig_ == NULL ? 0 : (*rs->obs_trig_) () ;
}


/** @brief Get the textual representation of the resource for the
 *	`/.well-known/casan` resource.
 *
 * The format of a "well-known"-type text is:
 *	<temp>;title="Temperature";rt="celcius"
 *
 * @param buf buffer where the textual representation must be stored
 * @param maxlen size of buffer
 * @return length of string (including final \0), or -1 if it not fits
 *	in the given buffer
 */

int well_known (Resource *rs , char *buf, size_t maxlen)
{
    int len ;
    
    len = sizeof "<>;title=..;rt=.." ;		// including '\0'
    len += strlen (rs->name_) + strlen (rs->title_) + strlen (rs->rt_) ;
    if (len > (int) maxlen)
		len = -1 ;
    else
		sprintf (buf, "<%s>;title=\"%s\";rt=\"%s\"", rs->name_, rs->title_, rs->rt_) ;

    return len ;
}



/** @brief For debugging purposes
 */

void printResource (Resource *rs)
{
    printf ("RES name = %s", rs->name_) ;
    printf (", title = %s",rs->title_) ;
    printf (", rt = %s", rs->rt_) ;
    printf("\n");
}
