#include "rime.h"
#include "../../libraries/L2-154/l2-154.h"



#define	CHANNEL		25
#define	PANID		CONST16 (0xca, 0xfe)
#define	MTU		0

PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);
l2addr_154 *destaddr;

char testpkt [] = "This is a very long test packet (more than 46 bytes) to send to a broadcast address" ;


void print_paylen_src_dst (void)
{
    l2addr_154 *src, *dst ;

    printf (" payload length= %d", get_paylen()) ;
    printf  (" ") ;
    src = get_src () ; 
    printAddr (src) ;
    printf (" -> ") ;
    dst = get_dst () ; 
    printAddr (dst) ;
    printf ("\n");
}


void recv_l2 (void)
{
    l2_recv_t r ;

    r = recv () ;

    switch (r)
    {
	case RECV_EMPTY :
		printf("rien\n");
	    break ;
	case RECV_OK :
	    printf ("OK") ;
	    print_paylen_src_dst () ;
	    dump_packet (0, 20) ;
	    break ;
	case RECV_WRONG_DEST :
	    printf ("WRONG DEST") ;
	    print_paylen_src_dst () ;
	    dump_packet (0, 20) ;
	    break ;
	case RECV_WRONG_TYPE :
	    printf ("WRONG TYPE") ;
	    print_paylen_src_dst () ;
	    dump_packet (0, 20) ;
	    break ;
	case RECV_TRUNCATED :
	    printf ("TRUNCATED") ;
	    print_paylen_src_dst () ;
	    break ;
	default :
	    printf ("UNKNOWN : r= %d", r) ;
	    printf(", payload length= %d\n", get_paylen()) ;
	    break ;
    }
}


void send_l2 (void)
{
    bool r ;

    r = send (bcastaddr (), (uint8_t *) testpkt, sizeof testpkt - 1) ;
    printf ("Sent broadcast: r = %d\n",r ) ;

    r = send (destaddr, (uint8_t *) testpkt, sizeof testpkt - 1) ;
    printf ("Sent unicast: r = %d\n",r ) ;
}

PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;
	l2addr_154 *myaddr = init_l2addr_154_char("45:67");
    destaddr = init_l2addr_154_char("12:34");

    PROCESS_BEGIN();
    setBroasdcastAddr();

    printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);

    printf ("%s : %s=", YELLOW ("OPTION"), RED ("optcode")) ;

    startL2_154(myaddr, CHANNEL, PANID);

    setMTU(MTU);
    etimer_set(&et,5*CLOCK_SECOND);

    while(1) {
    	etimer_set(&et,5*CLOCK_SECOND);
    	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    	send_l2 () ;
    	recv_l2 () ;
    	printf("\n");
    	printf("*************************************************************************");
    	printf("\n");
    }

    PROCESS_END();

}

