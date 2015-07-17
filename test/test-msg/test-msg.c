/*
 * Test program for the "message" and "option" classes
 *
 * Create and destroy messages with options
 * No packet is sent on the network
 */


 
#include "../../libraries/Casan/msg.h"
//#include "../../libraries/Casan/option.h"
#include "rime.h"

#define PATH1	"/path1"
#define PATH2	"/path2"

#define URIQUERY1 "uriquery=1"
#define URIQUERY2 "uriquery=2"
#define URIQUERY3 "uriquery=3"

#define	DEBUGINTERVAL	1




PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


void test_msg (void)
{
    l2net_154 *l2 ;

    printf ("STEP 1: create 2 empty messages\n") ;
    Msg *m1 = initMsg(l2) ;
    printMsg (m1) ;
    Msg *m2 = initMsg(l2) ;
    printMsg (m2) ;

    printf ("STEP 2: create options\n") ;
    option *oup1 = initOptionOpaque(MO_Uri_Path, (void *) PATH1, sizeof PATH1-1) ;
    option *oup2 = initOptionOpaque(MO_Uri_Path, (void *) PATH2, sizeof PATH2-1) ;
    option *ouq1 = initOptionOpaque(MO_Uri_Query, (void *) URIQUERY1, sizeof URIQUERY1-1) ;
    option *ouq2 = initOptionOpaque(MO_Uri_Query, (void *) URIQUERY2, sizeof URIQUERY2-1) ;
    option *ouq3 = initOptionOpaque(MO_Uri_Query, (void *) URIQUERY3, sizeof URIQUERY3-1) ;

    // REGISTER message
    set_type (m1, COAP_TYPE_CON) ;
    set_code (m1, COAP_CODE_GET) ;
    set_id (m1, 10) ;

   	printf ("STEP 3a: M1 add uriquery2\n") ;
    push_option (m1, ouq2) ;
    printMsg (m1) ;	printf("\n") ;

    printf ("STEP 3b: M1 add uripath1\n") ;
    push_option (m1, oup1) ;
    printMsg (m1) ;	printf("\n") ;

    printf  ("STEP 3c: M1 add uripath2\n") ;
    push_option (m1, oup2) ;
    printMsg (m1) ;	printf("\n") ;

    printf  ("STEP 3d: M1 add uriquery1\n") ;
    push_option (m1, ouq1) ;
    printMsg (m1) ;	printf("\n") ;

    printf("STEP 3e: M1 add uriquery3\n") ;
    push_option (m1, ouq3) ;
    printMsg (m1) ;	printf("\n") ;

    set_type (m2, COAP_TYPE_NON) ;
    set_code (m2, COAP_CODE_POST) ;
    set_id (m2, 11) ;

    printf ("STEP 3f: M2 add oriquery2\n") ;
    push_option (m2, ouq2) ;
    printMsg (m2) ;	printf("\n") ;

    if (get_errno () != 0)
    {
		printf  ("ERROR : ERRNO => ") ;
		printf ("%d\n",get_errno ()) ;
		reset_errno () ;
    }

    clock_delay (1000) ;
}

PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;
	PROCESS_BEGIN();

    printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);

    while(1) {     
    	test_msg();

    	printf("\n");
    	printf("*************************************************************************");
    	printf("\n");
        etimer_set(&et,5*CLOCK_SECOND); 
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }

    PROCESS_END();

}