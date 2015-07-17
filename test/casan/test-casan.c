/*
 * Example program for CASAN
 */

#include "../../libraries/L2-154/l2-154.h"
#include "../../libraries/Casan/casan.h"
 #include "lps331ap.h"



#define	CHANNEL	15
#define	PANID	CONST16 (0xca, 0xfe)
#define	MTU	127

#define	DEBUGINTERVAL	10
#define	SLAVEID		169


PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


uint8_t process_temp1 (Msg *in, Msg *out) 
{
    char payload [10] ;

    set_max_age (out, true, 0) ;		// answer is not cachable

    printf("process_temp1") ;

    int16_t value;
    lps331ap_read_temp(&value);
    value = 42.5 + value / 480.0 ;
    snprintf (payload, 10, "%d", value) ;

    set_payload_msg (out, (uint8_t *) payload,  strlen (payload)) ;

    return COAP_RETURN_CODE (2, 5) ;
}

uint8_t process_temp2 (Msg *in, Msg *out) 
{
    char payload [10] ;

    // out->max_age (true, 60) ;	// answer is cachable (default)

    printf("process_temp2") ;
    float value = isl29020_read_sample();

    snprintf (payload, 10, "%d", value) ;

    set_payload_msg (out, (uint8_t *) payload,  strlen (payload)) ;

    return COAP_RETURN_CODE (2, 5) ;
}


l2net_154 *l2;
l2addr_154 *myaddr;
Casan *ca;
Resource *r1;
Resource *r2;

PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;
	
	PROCESS_BEGIN();
		// LPS331AP pressure sensor initialisation
	    lps331ap_powerdown();
	    lps331ap_set_datarate(LPS331AP_P_12_5HZ_T_12_5HZ);

		myaddr = init_l2addr_154_char("23:34");

		l2 = startL2_154( myaddr, CHANNEL, PANID); 
		
		ca = initCasan(l2, MTU, SLAVEID);

		r1 = initResource ("t1", "Desk temp", "celsius") ;
		setHandlerResource(r1, COAP_CODE_GET, process_temp1 );
		register_resource(ca, r1);
		
		r2 = initResource ("t2", "Desk temp", "celsius") ;
		setHandlerResource(r2, COAP_CODE_GET, process_temp2 );
		register_resource(ca, r2);

		print_resources (ca) ;

		while(1) {    

			
	        etimer_set(&et,1.5*CLOCK_SECOND); 
        	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        	loop(ca);

	     }

 	PROCESS_END();

}