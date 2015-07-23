/**
 * @file casan.cpp
 * @brief Casan class implementation
 */

#include "casan.h"

#define	CASAN_NAMESPACE1	".well-known"
#define	CASAN_NAMESPACE2	"casan"
#define	CASAN_HELLO		"hello=%ld"
#define	CASAN_DISCOVER_SLAVEID	"slave=%ld"
#define	CASAN_DISCOVER_MTU	"mtu=%ld"
#define	CASAN_ASSOC_TTL		"ttl=%ld"
#define	CASAN_ASSOC_MTU		CASAN_DISCOVER_MTU

#define	CASAN_BUF_LEN		50	// > sizeof hello=.../slave=..../etc


#define CASAN_RESOURCES_ALL	"resources"



static struct
{
    const char *path ;
    int len ;
} casan_namespace [] =
{
    {  CASAN_NAMESPACE1, sizeof CASAN_NAMESPACE1 - 1 },
    {  CASAN_NAMESPACE2, sizeof CASAN_NAMESPACE2 - 1 },
} ;


/******************************************************************************
Constructor and simili-destructor
******************************************************************************/



/**
 * @brief Constructor
 *
 * The constructor method needs an already initialized L2 network
 * object, and a slave-id.
 *
 * @param l2 pointer to an already initialized network object (l2net-*)
 * @param mtu slave MTU (including MAC header and trailer) or 0 for default
 * @param slaveid unique slave-id
 */


Casan *initCasan (l2net_154 *l2, int mtu, long int slaveid)
{
    Casan *ca = (Casan *) malloc(sizeof (Casan));
    ca->l2_ = l2 ;
    ca->slaveid_ = slaveid ;
    curtime = 0 ;			// global variable
    ca->master_ = NULL;

    sync_time (&curtime) ;
    ca->defmtu_ = getMTU (l2) ;		// get default L2 MTU
    if (mtu > 0 && mtu < ca->defmtu_)
		ca->defmtu_ = mtu ;			// set a different default MTU
    reset_master (ca) ;			// master_ is reset (broadcast addr, mtu)
    ca->hlid_ = -1 ;
    ca->curid_ = 1 ;
    ca->retrans_ = initRetrans();
    master (ca->retrans_, &ca->master_) ;
    ca->status_ = SL_COLDSTART ;

    ca->reslist_ = NULL;
    

    return ca;
}



/*
 * @brief Reset CASAN engine
 *
 * This method is used to reset the CASAN engine.
 *
 * @bug as this method is not used, implementation is very rudimentary.
 *	One should restart all the engine, delete all exchange history, etc.
 */

void resetCasan (Casan *ca)
{
    ca->status_ = SL_COLDSTART ;
    ca->curid_ = 1 ;

    // remove resources from the list
    while (ca->reslist_ != NULL)
    {
		reslist *r ;

		r = ca->reslist_->next ;
		free(ca->reslist_) ;
		ca->reslist_ = r ;
    }

    resetRetrans (ca->retrans_) ;
    reset_master (ca) ;
}



/******************************************************************************
MTU handling
******************************************************************************/

/**
 * @brief Reset MTU value to user specified MTU
 *
 * This method resets the current MTU to the value specified by the
 * user (which is the default network MTU if supplied as 0).
 */

void reset_mtu (Casan *ca)
{
    negociate_mtu (ca, 0) ;
}

/**
 * @brief Change MTU value for a negociated value
 *
 * This method sets the current MTU to a value negociated between
 * the slave and the master. Parameter is the MTU value announced
 * by the master in its Assoc message, or 0 to reset the MTU to the
 * default value (which is the value specified by the user or the
 * default network specific MTU).
 */


void negociate_mtu (Casan *ca, int mtu)
{
    if (mtu > 0 && mtu <= ca->defmtu_)
		ca->curmtu_ = mtu ;
    else
		ca->curmtu_ = ca->defmtu_ ;		// reset MTU to default value
    setMTU (ca->l2_, ca->curmtu_) ;		// notify L2 network
}



/******************************************************************************
Master handling
******************************************************************************/

/**
 * Reset master coordinates to an unknown master:
 * * address is null
 * * hello-id is -1 (i.e. unknown hello-id)
 * * current MTU is reset to default (user initialized) MTU
 */

void reset_master (Casan *ca)
{	
    if (ca->master_ != NULL)
		freel2addr_154(ca->master_) ; 

    ca->master_ = NULL ;
    ca->hlid_ = -1 ;
    reset_mtu (ca) ;			// reset MTU to default
    printf ("Master reset to broadcast address and default MTU\n") ;
}



/**
 * Does master address match the given address (which cannot be a
 * NULL pointer)?
 */

bool same_master (Casan *ca, l2addr_154 *a)
{
    return ca->master_ != NULL && isEqualAddr(a, ca->master_ );
}


/**
 * Change master to a known master.
 * - address is taken from the current incoming message
 * - hello-id is given, may be -1 if value is currently not known
 * - mtu is given, may be -1 if current value must not be changed
 */

void change_master (Casan *ca, long int hlid, int mtu)
{
    l2addr_154 *newmaster ;

    newmaster = get_src (ca->l2_) ;	// get a new address
    if (ca->master_ != NULL)
    {
		if (isEqualAddr(newmaster, ca->master_))
		{
		    if (hlid != -1)
				ca->hlid_ = hlid ;
		    freel2addr_154(newmaster) ;
		}
		else
		{
		    freel2addr_154(ca->master_) ;
		    ca->master_ = newmaster ;
		    ca->hlid_ = hlid ;
		}
    }
    else
    {
		ca->master_ = newmaster ;
		ca->hlid_ = hlid ;
    }

    if (mtu != -1)
		negociate_mtu (ca, mtu) ;

    printf ("Master set to ") ;
    printAddr (ca->master_) ;
    printf (", helloid= %d", ca->hlid_) ;
    printf (", mtu= %d", ca->curmtu_) ;
    printf("\n");
}



/******************************************************************************
Resource handling
******************************************************************************/

/**
 * @brief Register a resource to the CASAN engine
 *
 * This method is used to register a resource with the CASAN engine.
 * This resource will then be advertised with the `/.well-known/casan`
 * resource during the next association or with a specific request
 * from the master. Thus, registering a resource after an association
 * (when the slave returns an association answer containing the
 * `/.well-known/casan`) will not provoke a new association. One must
 * wait the next association renewal for the resource to be published
 * and thus known by the master.
 *
 * @param res Address of the resource to register
 */

void register_resource (Casan *ca, Resource *res)
{
    reslist *newr, *prev, *cur ;

    /*
     * Register resource in last position of the list to respect
     * order provided by the application
     */

    newr = (reslist *) malloc(sizeof (reslist)) ;
    newr->res = res ;

    prev = NULL ;
    for (cur = ca->reslist_ ; cur != NULL ; cur = cur->next)
		prev = cur ;
    if (prev != NULL)
		prev->next = newr ;
    else
		ca->reslist_ = newr ;
    newr->next = NULL ;
}


/**
 * @brief Process an incoming message requesting for a resource
 *
 * This methods:
 * * analyze uri_path option to find the resource
 * * either give answer if this is the /resources URI
 * * or call the handler for user-defined resources
 * * or return 4.04 code
 * * pack the answer in the outgoing message
 *
 * This method is made public for testing purpose.
 *
 * @bug Only one level of path is allowed (i.e. /a, and not /a/b nor /a/b/c)
 *
 * @param in Incoming message
 * @param out Message which will be sent in return
 */

void process_request (Casan *ca, Msg *in, Msg *out) 
{
    option *o ;
    bool rfound = false ;		// resource found

    reset_next_option (in) ;
    for (o = next_option (in) ; o != NULL ; o = next_option (in))
    {
		if (getOptcode (o) == MO_Uri_Path)
		{
		    // request for all resources
		    if (getOptlen (o) == (int) (sizeof CASAN_RESOURCES_ALL - 1)
			&& memcmp (getOptval (o, (int *) 0), CASAN_RESOURCES_ALL, 
					    sizeof CASAN_RESOURCES_ALL - 1) == 0)
		    {
				rfound = true ;
				set_type (out, COAP_TYPE_ACK) ;
				set_id (out, get_id (in)) ;
				set_token_msg (out, get_token_msg (in)) ;
				set_code (out, COAP_CODE_OK) ;
				(void) get_well_known (ca, out) ;
		    }
		    else
		    {
				Resource *res ;

				// we benefit from the added '\0' at the end of an option
				res = get_resource (ca, (char *) getOptval (o,(int *) 0)) ;
				if (res != NULL)
				{
				    option *obs ;
				    uint32_t obsval ;

				    rfound = true ;

				    obs = search_option (in, MO_Observe) ;
				    if (obs != NULL)
						obsval = getOptvalInteger (obs) ;

				    if (obs != NULL && obsval == 0)
						observedResource (res, true, in) ;
				    else
						observedResource (res, false, NULL) ;

				    set_type (out, COAP_TYPE_ACK) ;
				    set_id (out, get_id (in)) ;
				    set_token_msg (out, get_token_msg (in)) ;

				    if (obs != NULL && obsval == 0)
				    {
						option *robs = initOption();

						setOptcode (robs, MO_Observe) ;
						setOptvalInteger (robs,next_serial (res)) ;
						push_option (out, robs) ;
				    }

				    request_resource (in, out, res) ;
				}
		    }
		    break ;
		}
    }

    if (! rfound)
    {
		set_type (out, COAP_TYPE_ACK) ;
		set_id (out, get_id (in)) ;
		set_token_msg (out, get_token_msg (in)) ;
		set_code (out, COAP_CODE_NOT_FOUND) ;
    }
}



/**
 * Build a response message
 *
 * Build a CASAN response message for an incoming request or when
 * an observed resource triggered an event. Part of the message
 * is already built (type, id, token, observe option), answer must
 * be completed by the application handler.
 *
 * @param pin pointer to incoming message or NULL
 * @param pout pointer to the output message being built
 * @param res addressed resource
 */

void request_resource (Msg *pin, Msg *pout, Resource *res)
{
    handler_res_t h ;
    uint8_t code ;

    h = getHandlerResource (res, (coap_code_t) get_code (pin)) ;
    if (h == NULL)
    {
		code = COAP_CODE_BAD_REQUEST ;
    }
    else
    {
		// add Content Format option
		set_content_format (pout, false, cf_text_plain) ;
		code = (*h) (pin, pout) ;
    }
    set_code (pout, code) ;
}


/**
 * Check all observed resources in order to detect changes and
 * send appropriate observe message.
 *
 * @param out an output message
 */

void check_observed_resources (Casan *ca, Msg *out)
{
    Resource *res ;
    reslist *rl ;

    for (rl = ca->reslist_ ; rl != NULL ; rl = rl->next)
    {
		res = rl->res ;
		if (check_trigger (res))
		{
		    resetMsg (out) ;

		    set_type (out, COAP_TYPE_ACK) ;
		    set_token_msg (out, get_token (res)) ;

		    option *obs = initOptionInteger(MO_Observe, next_serial (res)) ;
		    push_option (out, obs) ;

		    request_resource (NULL, out, res) ;
		}
    }
}



bool get_well_known (Casan *ca, Msg *out) 
{
    char *buf ;
    size_t size ;
    reslist *rl ;
    size_t avail ;
    bool reset ;

    reset = false ;
    set_content_format (out, reset, cf_text_plain) ;
    //printMsg(out );
    avail = avail_space (out) ;	
    buf = (char *) malloc (avail) ;

    size = 0 ;
    for (rl = ca->reslist_ ; rl != NULL ; rl = rl->next) 
    {
		int len ;

		if (size > 0)			// separator "," between resources
		{
		    if (size + 2 < avail)
		    {
				buf [size++] = ',' ;
				buf [size] = '\0' ;
		    }
		    else break ;		// too large
		}

		len = well_known (rl->res, buf + size, avail - size) ;
		if (len == -1)
		    break ;

		size += len - 1 ;		// exclude '\0'
    }

    set_payload_msg (out, (uint8_t *) buf, size) ;
    free (buf) ;


    /*
     * Did all resources fitted in the message, or do we left the loop
     * before its term?
     */

    if (rl != NULL)
    {
		printf ("%s",B_RED "Resource '") ;
		printf ("%s",get_name (rl->res)) ;
		printf ("' do not fit in buffer of %d", avail) ;
		printf (" bytes %s\n", C_RESET) ;
    }

    return rl == NULL ;			// true if all res are in the message
}


/**
 * Find a particular resource by its name
 */

Resource *get_resource (Casan *ca, const char *name)
{
    reslist *rl ;

    for (rl = ca->reslist_ ; rl != NULL ; rl = rl->next)
    	if (strcmp (name, get_name (rl->res)) == 0)
			break ;
    return rl != NULL ? rl->res : NULL ;
}



/******************************************************************************
Main CASAN loop
******************************************************************************/

/**
 * @brief Main CASAN loop
 *
 * This method must be called regularly (typically in the loop function
 * of the Arduino framework) in order to process CASAN events.
 */

void loop (Casan *ca)
{
	
    Msg *in = initMsg(ca->l2_) ;
    Msg *out = initMsg(ca->l2_) ;
    l2_recv_t ret ;
    uint8_t oldstatus ;
    long int hlid = 0;
    l2addr_154 *srcaddr ;
    int mtu ;				// mtu announced by master in assoc msg

    oldstatus = ca->status_ ;		// keep old value for debug display
    sync_time (&curtime) ;		// get current time
    loopRetrans (ca->retrans_, ca->l2_, &curtime) ;	// check needed retransmissions

    srcaddr = NULL ;

    ret = recvMsg (in) ;			// get received message
    if (ret == RECV_OK)
		srcaddr = get_src (ca->l2_) ;	// get a new address

    switch (ca->status_)
    {
	case SL_COLDSTART :
	    send_discover (ca, out) ;
	    ca->twait_ = initTwait (&curtime) ;
	    ca->status_ = SL_WAITING_UNKNOWN ;
	    break ;

	case SL_WAITING_UNKNOWN :
	    if (ret == RECV_OK)
	    {	
			check_msg_received (ca->retrans_, in) ;

			if (is_ctl_msg (in))
			{	printMsg(in);
			    if (is_hello (in, &hlid))
			    {
					printf("Received a CTL HELLO msg\n") ;
					change_master (ca, hlid, -1) ;	// don't change mtu
					ca->twait_ = initTwait (&curtime) ;
					ca->status_ = SL_WAITING_KNOWN ;
			    }
			    else if (is_assoc (in, &ca->sttl_, &mtu))
			    {

					printf ("Received a CTL ASSOC msg UNKNOWN\n") ;
					change_master (ca, -1, mtu) ;	// "unknown" hlid
					send_assoc_answer (ca, in, out) ;
					ca->trenew_ = initTrenew (&curtime, ca->sttl_) ;
					ca->status_ = SL_RUNNING ;
			    }
			    else printf ("%s\n",RED ("Unkwnon CTL")) ;
			}

	    }
	    if (ca->status_ == SL_WAITING_UNKNOWN && nextTwait (ca->twait_, &curtime)){
			send_discover (ca, out) ;
		}
	
	    break ;

	case SL_WAITING_KNOWN :
	    if (ret == RECV_OK)
	    {		
			check_msg_received (ca->retrans_, in) ;

			if (is_ctl_msg (in))
			{
			    if (is_hello (in, &hlid))
			    {
					printf ("Received a CTL HELLO msg\n") ;
					change_master (ca, hlid, -1) ;	// don't change mtu
			    }
			    else if (is_assoc (in, &ca->sttl_, &mtu))
			    {
					printf ("Received a CTL ASSOC msg KNOWN\n") ;
					change_master (ca, -1, mtu) ;	// unknown hlid
					send_assoc_answer (ca, in, out) ;
					ca->trenew_ = initTrenew (&curtime, ca->sttl_) ;
					ca->status_ = SL_RUNNING ;
			    }
			    else printf ("%s\n", RED ("Unkwnon CTL")) ;
			}
	    }

	    if (ca->status_ == SL_WAITING_KNOWN)
	    {
			if (expiredTwait (ca->twait_, &curtime))
			{
			    reset_master (ca) ;		// master_ is no longer known
			    send_discover (ca, out) ;
			    ca->twait_ = initTwait (&curtime) ;	// reset timer
			    ca->status_ = SL_WAITING_UNKNOWN ;
			}
			else if (nextTwait (ca->twait_, &curtime))
			{
			    send_discover (ca, out) ;
			}
	    }

	    break ;

	case SL_RUNNING :
	case SL_RENEW :
	    if (ret == RECV_OK)
	    {	
			check_msg_received (ca->retrans_, in) ;

			if (is_ctl_msg (in))
			{
			    if (is_hello (in, &hlid))
			    {
					printf ("Received a CTL HELLO msg\n") ;
					if (! same_master (ca, srcaddr) || hlid != ca->hlid_)
					{
					    int oldhlid = ca->hlid_ ;

					    change_master (ca, hlid, 0) ;	// reset mtu
					    if (oldhlid != -1)
					    {
							ca->twait_ = initTwait (&curtime) ;
							ca->status_ = SL_WAITING_KNOWN ;
					    }
					}
			    }
			    else if (is_assoc (in, &ca->sttl_, &mtu))
			    {
					printf ("Received a CTL ASSOC msg RENEW\n") ;
					if (same_master (ca, srcaddr))
					{
					    negociate_mtu (ca, mtu) ;
					    send_assoc_answer (ca, in, out) ;
					    ca->trenew_ = initTrenew (&curtime, ca->sttl_) ;
					    ca->status_ = SL_RUNNING ;
					}
			    }
			    else printf ("%s\n",RED ("Unkwnon CTL")) ;
			}
			else		// request for a normal resource
			{
			    // deduplicate () ;
			    process_request (ca, in, out) ;
			    sendMsg (out, ca->master_) ;
			}
	    }
	    else if (ret == RECV_TRUNCATED)
	    {
			printf ("%s", RED ("Request too large")) ;
			set_type (out, COAP_TYPE_ACK) ;
			set_id (out, get_id (in)) ;
			set_token_msg (out, get_token_msg (in)) ;
			option *o = initOptionInteger(MO_Size1, getMTU (ca->l2_)) ;
			push_option (out, o) ;
			set_code (out, COAP_CODE_TOO_LARGE) ;
			sendMsg (out, ca->master_) ;
	    }

	    check_observed_resources (ca, out) ;
	    //printf("ici fin\n");
	    //printf("%d   %d  \n",curtime , &curtime);
	    if (ca->status_ == SL_RUNNING && renewTrenew (ca->trenew_, &curtime))
	    {
	    	
			send_discover (ca, out) ;
			ca->status_ = SL_RENEW ;
	    }

	    if (ca->status_ == SL_RENEW && nextTrenew (ca->trenew_, &curtime))
	    {
	    	
			send_discover (ca, out) ;
	    }

	    if (ca->status_ == SL_RENEW && expiredTrenew (ca->trenew_, &curtime))
	    {
			reset_master (ca) ;	// master_ is no longer known
			send_discover (ca, out) ;
			ca->twait_ = initTwait ( &curtime) ;	// reset timer
			ca->status_ = SL_WAITING_UNKNOWN ;
	    }

	    break ;

	default :
	    printf ("Error : casan status not known : ") ;
	    printf ("%d\n",ca->status_) ;
	    break ;
    }

    if (oldstatus != ca->status_)
    {
		printf ("Status: %s ", C_GREEN) ;
		print_status(oldstatus);
		printf ("%s -> %s", C_RESET, C_GREEN) ;
		print_status (ca->status_) ;
		printf("%s\n", C_RESET) ;
		printf("\n");
    }

    if (srcaddr != NULL)
		freel2addr_154(srcaddr) ;

}



/******************************************************************************
Recognize control messages
******************************************************************************/

/**
 * Is the incoming message an CASAN control message?
 * Just verify if Uri_Path options match the casan_namespace [] array
 * in the right order
 */

bool is_ctl_msg (Msg *m)
{
    int i = 0 ;

    reset_next_option (m) ;
    option *o;
    for ( o = next_option (m) ; o != NULL ; o = next_option (m))
    {
		if (getOptcode (o) == MO_Uri_Path)
		{
		    if (i >= NTAB (casan_namespace))
				return false ;
		    if (casan_namespace [i].len != getOptlen (o))
				return false ;
		    if (memcmp (casan_namespace [i].path, getOptval (o, (int *) 0), getOptlen (o)))
				return false ;
		    i++ ;
		}
    }
    reset_next_option (m) ;
    if (i != NTAB (casan_namespace))
		return false ;

    return true ;
}


/**
 * Check if the control message is a Hello message from the master
 * and returns the contained hello-id
 */

bool is_hello (Msg *m, long int *hlid)
{
    bool found = false ;

    // a hello msg is NON POST
    if (get_type (m) == COAP_TYPE_NON && get_code (m) == COAP_CODE_POST)
    {
		reset_next_option (m) ;
		option *o;
		for ( o = next_option (m) ; o != NULL ; o = next_option (m))
		{
		    if (getOptcode (o) == MO_Uri_Query)
		    {
				// we benefit from the added nul byte at the end of val
				if (sscanf ((const char *) getOptval (o, (int *) 0), CASAN_HELLO, hlid) == 1)
				    found = true ;
		    }
		}
    }

    return found ;
}



/**
 * Check if the control message is an Assoc message from the master
 * and returns the contained slave-ttl
 */

bool is_assoc (Msg *m, time_t *sttl, int *mtu)
{
    bool found_ttl = false ;
    bool found_mtu = false ;

    if (get_type (m) == COAP_TYPE_CON && get_code (m) == COAP_CODE_POST)
    {
		reset_next_option (m) ;
		option *o;
		for ( o = next_option (m) ; o != NULL ; o = next_option (m))
		{
		    if (getOptcode (o) == MO_Uri_Query)
		    {
				long int n ;		// sscanf "%ld" waits for a long int

				// we benefit from the added nul byte at the end of val
				if (sscanf ((const char *) getOptval (o, (int *) 0), CASAN_ASSOC_TTL, &n) == 1)
				{
				    printf ("%s%d\n",BLUE ("TTL recv: "), n) ;
				    *sttl = ((time_t) n) * 50 ;
				    found_ttl = true ;
				    // continue, just in case there are other query strings
				}
				else if (sscanf ((const char *) getOptval (o, (int *) 0), CASAN_ASSOC_MTU, &n) == 1)
				{
				    printf ("%s%d\n",BLUE ("MTU recv: "), n) ;
				    *mtu = n ;
				    found_mtu = true ;
				    // continue, just in case there are other query strings
				}
				else break ;
		    }
		}
    }

    return found_ttl && found_mtu ;
}



/******************************************************************************
Send control messages
******************************************************************************/

/**
 * Initialize an "empty" control message
 * Just add the Uri_Path options from the casan_namespace [] array
 */

void mk_ctl_msg (Msg *out)
{
    int i ;

    for (i = 0 ; i < NTAB (casan_namespace) ; i++)
    {
		option *path = initOptionOpaque(MO_Uri_Path, (void *) casan_namespace [i].path,
						casan_namespace [i].len) ;
		push_option (out, path) ;
    }
}



/**
 * Send a discover message
 */

void send_discover (Casan *ca, Msg *out)
{
    char tmpstr [CASAN_BUF_LEN] ;
    l2addr_154 *dest ;

    printf ("Sending Discover\n") ;
    
    resetMsg (out) ;
    set_id (out, ca->curid_++) ;
    set_type (out, COAP_TYPE_NON) ;
    set_code (out, COAP_CODE_POST) ;
    mk_ctl_msg (out) ;

    snprintf (tmpstr, sizeof tmpstr, CASAN_DISCOVER_SLAVEID, ca->slaveid_) ;
    option *o1 = initOptionOpaque(MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    push_option (out, o1) ;


    snprintf (tmpstr, sizeof tmpstr, CASAN_DISCOVER_MTU, (long int) ca->defmtu_) ;
    option *o2 = initOptionOpaque(MO_Uri_Query, tmpstr, strlen (tmpstr)) ;
    push_option (out, o2) ;

    dest = (ca->master_ != NULL) ? ca->master_ : bcastaddr () ;
    //printMsg(out);
    sendMsg (out, dest) ;

    freeOption(o1);
    freeOption(o2);

}


/**
 * Send the answer to an association message
 * (the association task itself is handled in the CASAN main loop)
 */

void send_assoc_answer (Casan *ca, Msg *in, Msg *out)
{
    l2addr_154 *dest ;

    dest = get_src (ca->l2_) ;

    // send back an acknowledgement message
    set_type (out, COAP_TYPE_ACK) ;
    set_code (out, COAP_CODE_OK) ;
    set_id (out, get_id (in)) ;

    // will get the resources and set them in the payload in the right format
    (void) get_well_known (ca, out) ;

    // send the packet
    if (! sendMsg (out, dest))
		printf ("%s", RED ("Cannot send the assoc answer message")) ;

    freel2addr_154(dest) ;
}


/******************************************************************************
Debug methods
******************************************************************************/

/**
 * @brief Print the list of resources, used for debug purpose
 */

void print_resources (Casan *ca)
{
    reslist *rl ;

    for (rl = ca->reslist_ ; rl != NULL ; rl = rl->next)
		printResource (rl->res) ;
}



void print_coap_ret_type (l2_recv_t ret)
{
    switch (ret)
    {
		case RECV_WRONG_DEST :
		    printf ("RECV_WRONG_DEST\n") ;
		    break ;
		case RECV_WRONG_TYPE :
		    printf ("RECV_WRONG_TYPE\n") ;
		    break ;
		case RECV_OK :
		    printf ("RECV_OK\n") ;
		    break ;
		default :
		    printf ("ERROR RECV\n") ;
		    break ;
    }
}


void print_status (uint8_t status)
{
    switch (status)
    {
		case SL_COLDSTART :
		    printf("SL_COLDSTART") ;
		    break ;
		case SL_WAITING_UNKNOWN :
		    printf ("SL_WAITING_UNKNOWN") ;
		    break ;
		case SL_WAITING_KNOWN :
		    printf ("SL_WAITING_KNOWN") ;
		    break ;
		case SL_RENEW :
		    printf ("SL_RENEW") ;
		    break ;
		case SL_RUNNING :
		    printf ("SL_RUNNING") ;
		    break ;
		default :
		    printf("???") ;
		    printf ("%d",status) ;
		    break ;
    }
}