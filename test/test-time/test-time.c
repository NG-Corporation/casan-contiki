#include "../../libraries/Casan/time.h"
#include "rime.h"

/*
 * Test program for the "time" class
 */

void test_diff (void)
{
    time_t x = 0 ;
    timediff_t diff ;

    printf ("x ") ;
    print_time(&x);
    printf("\n");

    printf ("curtime : " ) ;
    sync_time (&curtime) ;
    print_time (&curtime) ;
    printf("\n");

    printf ("diff : ") ;
    diff = x - curtime ;
    printf ("%d\n", (long int) diff) ;

    if (curtime < x)
		printf ("\033[31m ISSUE : curtime < x \033[00m \n") ;

    if (x < curtime)
		printf  ("\033[32m OK : x < curtime \033[00m \n") ;

}


// void test_operators (void)
// {
//     time_t x, y ;
//     timediff_t d ;

//     printf ("x : ") ;
//     print_time (&x) ;

//     printf ("x = curtime \n") ;
//     x = curtime ;

//     printf ("x : ") ;
//     print_time (&x) ;
//     printf("\n"); 

//     printf("curtime : ") ;
//     print_time (&curtime) ;
//     printf("\n"); 

//     printf ("x + 5000 ; \n") ;
//     x = x + 5000 ;
//     printf ("x : ") ;
//     print_time (&x) ;
//     printf("\n");

//     printf ("y = x ; y = y + 5000 ; \n") ;
//     y = x ;
//     y = y + 5000 ;
//     printf ("y : ") ;
//     print_time (&y) ;
//     printf("\n"); ;
//     printf ("x : ") ;
//     print_time (&x) ;
//     printf("\n"); ;

//     printf ("d = y - x : \n") ;
//     d = y - x ;
//     printf ("%lu\n", (unsigned long int) d) ;

//     printf ("y = y - 5000 ;\n") ;
//     y = y - 5000 ;
//     print_time (&y) ;
//     printf("\n"); 

//     sync_time (&curtime) ;
// }


PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);



PROCESS_THREAD(test, ev, data)
{
	static struct etimer et;
	

	PROCESS_BEGIN();
    sync_time (&curtime) ;

	while(1) {     

		unsigned long t = clock_time() ;
		printf ("MILLIS : %d\n", t) ;
		test_diff () ;
		//test_operators () ;

    	printf("\n");

    	printf("*************************************************************************");
    	printf("\n");
        etimer_set(&et,5*CLOCK_SECOND); 
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }

    PROCESS_END();

}