#include "contiki.h"
#include <string.h>
#include "rime.h"
//#include "libraries/ConMsg/ConMsg.h"




PROCESS(test, "bah...test !");
AUTOSTART_PROCESSES(&test);



char testpkt [] = "This is a very long test packet (more than 46 bytes) to send to a broadcast address" ;
char testpkt2 [] = "brun the world";

PROCESS_THREAD(test, ev, data)
{
    
    static struct etimer et;
    static int len = 100;

    PROCESS_BEGIN();

    printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);

    etimer_set(&et,2*CLOCK_SECOND);

    start();
    printf("ok\n");
    if(sendto((uint8_t *) testpkt, sizeof testpkt - 1)) {
        printf("message sent\n");
    }
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if (sendto((uint8_t *) testpkt2, sizeof testpkt2 - 1)){
        printf("message sent\n");
    }
    printf("c'est ok\n");


    PROCESS_END();
}