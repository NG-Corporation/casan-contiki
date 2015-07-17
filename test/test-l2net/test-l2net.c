#include "rime.h"
#include "../../libraries/L2-154/l2-154.h"



#define	CHANNEL		13
#define	PANID		CONST16 (0xca, 0xfe)
#define	MTU		125

PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


char testpkt [] = "This is a very long test packet (more than 46 bytes) to send to a broadcast address" ;

void print_paylen_src_dst (l2net_154 *l2)
{
    l2addr_154 *src, *dst ;

    printf (" payload length= %d", get_paylen(l2)) ;
    printf  (" ") ;
    src = get_src (l2) ; 
    printAddr (src) ;
    printf (" -> ") ;
    dst = get_dst (l2) ; 
    printAddr (dst) ;
    printf ("\n");
}


void recv_l2 (l2net_154 *l2)
{
    l2_recv_t r ;

    r = recv (l2) ;

    switch (r)
    {
	case RECV_EMPTY :
		printf("rien\n");
	    break ;
	case RECV_OK :
	    printf ("OK") ;
	    print_paylen_src_dst (l2) ;
	    dump_packet (l2, 0, 20) ;
	    break ;
	case RECV_WRONG_DEST :
	    printf ("WRONG DEST") ;
	    print_paylen_src_dst (l2) ;
	    dump_packet (l2, 0, 20) ;
	    break ;
	case RECV_WRONG_TYPE :
	    printf ("WRONG TYPE") ;
	    print_paylen_src_dst (l2) ;
	    dump_packet (l2, 0, 20) ;
	    break ;
	case RECV_TRUNCATED :
	    printf ("TRUNCATED") ;
	    print_paylen_src_dst (l2) ;
	    break ;
	default :
	    printf ("UNKNOWN : r= %d", r) ;
	    printf(", payload length= %d\n", get_paylen(l2)) ;
	    break ;
    }
}


void send_l2 (l2net_154 *l2, l2addr_154 *destaddr)
{

    bool r ;

    // r = send (l2, bcastaddr (), (uint8_t *) testpkt, sizeof testpkt - 1) ;
    // printf ("Sent broadcast: r = %d\n",r ) ;

    // printAddr(destaddr);
    //     printf("\n");



    r = send (l2, destaddr, (uint8_t *) testpkt, sizeof testpkt - 1) ;
    printf ("Sent unicast: r = %d\n",r ) ;
}

l2addr_154 *myaddr;
l2addr_154 *destaddr;
l2net_154 *l2;

PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;

    PROCESS_BEGIN();
    myaddr = init_l2addr_154_char("45:67");
    destaddr = init_l2addr_154_char("12:34");
    l2 = startL2_154( myaddr, CHANNEL, PANID);  

    setMTU(l2, MTU);

    printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);

    printf ("%s : %s=", YELLOW ("OPTION"), RED ("optcode")) ;

    
    
    
    while(1) {     
    	send_l2 (l2, destaddr) ;
        recv_l2(l2);

    	printf("\n");
    	printf("*************************************************************************");
    	printf("\n");
        etimer_set(&et,5*CLOCK_SECOND); 
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }

    PROCESS_END();

}

