#include "rime.h"
#include "../../libraries/L2-154/l2-154.h"


 PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);



char text[] = "Hello World, sent by the ABC module, part of the Rimer "
       "communication stack, I need a longer text so I'll just "
       "write something";

PROCESS_THREAD(test, ev, data)
{
    //static struct etimer et;
    static int len = 100;
    l2addr_154 *x = init_l2addr_154_char("ca:fe");
    l2addr_154 *y = init_l2addr_154_char("be:ef");

    PROCESS_BEGIN();

    printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                         rimeaddr_node_addr.u8[1]);

    //etimer_set(&et,5*CLOCK_SECOND);

    if(isEqualAddr(x,y)) {
        print(x);
        print(y);
        printf("x == y (problem)\n");
    }else{
        print(x);
        print(y);
        printf("x != y (ok)\n" );
    }
    copyAddr(x,y);
    if(isEqualAddr(x,y)) {
        print(x);
        print(y);
        printf("x == y (OK))\n");
    }else{
        print(x);
        print(y);
        printf("x != y (BAD))\n" );
    }



    PROCESS_END();
}