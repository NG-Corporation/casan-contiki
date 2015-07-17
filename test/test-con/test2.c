#include "contiki.h"
#include "rime.h"
#include "../../libraries/ConMsg/ConMsg.h"


#define	BYTE_HIGH(n)		(((n) & 0xff00) >> 8)
#define	BYTE_LOW(n)		((n) & 0xff)


PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;

	ConReceivedFrame *r;
	PROCESS_BEGIN();

	printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);
	conmsg = (ConMsg * ) malloc (sizeof(ConMsg));
	setChannel(17);
	
	start();

	while(1){
		etimer_set(&et,20*CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		r = get_received() ;
		printf("%s  : %d\n",r->payload, r->paylen );
		printf("dest : ");
		printf("%x",BYTE_LOW(r->dstaddr));
		printf(" : " );
		printf("%x\n",BYTE_HIGH(r->dstaddr) );
		printf("src : ");
		printf("%x",BYTE_LOW(r->srcaddr));
		printf(" : " );
		printf("%x\n",BYTE_HIGH(r->srcaddr) );

		skip_received();
		r = get_received() ;
		printf("%s  : %d\n",r->payload, r->paylen );
		printf("dest : ");
		printf("%x",BYTE_LOW(r->dstaddr));
		printf(" : " );
		printf("%x\n",BYTE_HIGH(r->dstaddr) );
		printf("src : ");
		printf("%x",BYTE_LOW(r->srcaddr));
		printf(" : " );
		printf("%x\n",BYTE_HIGH(r->srcaddr) );
		skip_received();
	}

	PROCESS_END();
}


