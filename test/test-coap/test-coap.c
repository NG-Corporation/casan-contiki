/*
 * Test program for sending and receiving CASAN/CoAP messages
 */

#define	PATH1		".well-known"
#define	PATH2		"casan"
#define	PATH3		"path3"

#define	DEBUGINTERVAL	5

#include "../../libraries/L2-154/l2-154.h"
#include "../../libraries/Casan/msg.h"


#define	CHANNEL	26
#define	PANID	CONST16 (0xca, 0xfe)
#define	MTU	125


PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


void test_recv (l2net_154 *l2)
{
    Msg *in = initMsg(l2) ;
    l2_recv_t r ;

    printf("\n");
    while ((r = recvMsg (in)) == RECV_OK) {
	   printMsg (in) ;
       printf("\n");
   }
}


void res_send (int msgnum, bool ok)
{
    printf ("Sending message  %d : %d\n", msgnum, ok) ;
}


void test_send (l2net_154 *l2, l2addr_154 *dest)
{
    Msg *m1 = initMsg(l2) ;
    Msg *m2 = initMsg(l2) ;
    bool ok ;

    option *up1 = initOptionOpaque(MO_Uri_Path, PATH1, sizeof PATH1 - 1) ;
    option *up2 = initOptionOpaque(MO_Uri_Path, PATH2, sizeof PATH2 - 1) ;
    option *up3 = initOptionOpaque(MO_Uri_Path, PATH3, sizeof PATH3 - 1) ;
    option *ocf = initOptionOpaque(MO_Content_Format, "abc", sizeof "abc" - 1) ;

    set_id (m1, 258) ;
    set_type (m1, COAP_TYPE_NON) ;
    push_option (m1, ocf) ;
    push_option (m1, up1) ;
    push_option (m1, up2) ;
    push_option (m1, up3) ;
    ok = sendMsg (m1, dest) ;
    res_send (1, ok) ;

    set_id (m2, 33) ;
    set_type (m2, COAP_TYPE_CON) ;
    push_option (m2, ocf) ;
    ok = sendMsg (m2, dest) ;
    res_send (2, ok) ;
}

l2addr_154 *myaddr;
l2addr_154 *dest;
l2net_154 *l2;

PROCESS_THREAD(test, ev, data)
{

	static struct etimer et;

	PROCESS_BEGIN();
		myaddr = init_l2addr_154_char("45:67");
	    l2 = startL2_154( myaddr, CHANNEL, PANID); 
	    dest = l2addr_154_broadcast;
	    setMTU(l2, MTU);
		while(1) {    

			test_send (l2, dest) ;
	    	test_recv (l2) ;
			
	    	printf("\n");
	    	printf("*************************************************************************");
	    	printf("\n");

	        etimer_set(&et,5*CLOCK_SECOND); 
        	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	    }

    PROCESS_END();

}