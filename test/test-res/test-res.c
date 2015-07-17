/*
 * Test program for CASAN resource management
 */
#include "../../libraries/L2-154/l2-154.h"
 #include "../../libraries/Casan/casan.h"

#define	CHANNEL		17
#define	PANID		CONST16 (0xca, 0xfe)
#define	MTU		0


#define R1_name		"light"
#define R1_title	"light"
#define R1_rt		"light"

#define R2_name		"temp"
#define R2_title	"temperature"
#define R2_rt		"°c"

#define PATH_WK		".well-known"
#define	PATH_CASAN	"casan"


PROCESS(test, "bah... test !");
AUTOSTART_PROCESSES(&test);


uint8_t process_light (Msg *in, Msg *out)
{
    printf ("process_light\n") ;
    set_payload_msg (out, (uint8_t *) "on", 2) ;		// light is "on"
    return COAP_RETURN_CODE (2, 5) ; ;
}

uint8_t process_temp (Msg *in, Msg *out)
{
    printf ("process_temp\n");
    set_payload_msg (out, (uint8_t *) "23.5", 4) ;		// temp is hot ;-)
    return COAP_RETURN_CODE (2, 5) ;
}



void test_resource (Casan *ca, l2net_154 *l2, const char *name) {
	Msg *in = initMsg(l2) ;
    Msg *out = initMsg(l2) ;

    printf("Resource: '%s'\n", name);

    option *up = initOptionOpaque(MO_Uri_Path, (void *) name, strlen (name)) ;
    option *ocf = initOptionOpaque(MO_Content_Format, (void *) "abc", sizeof "abc" - 1) ;

    set_id(in, 100);
    set_type(in, COAP_TYPE_ACK);
    push_option(in, up);
    push_option(in, ocf);
    printf("Simulated message IN: \n");
    printMsg(in);

    process_request(ca, in ,out);

    printf("Simulated message OUT: \n");
    printMsg(out);
    printf("Done\n");
}


int slaveid = 169 ;
bool promisc = false ;

const char *resname [] =
{
    "resources",
    "nonexistant",
    R1_name,
    R2_name,
} ;

l2net_154 *l2;
l2addr_154 *myaddr;
Casan *ca;
Resource *r1;
Resource *r2;
static int n = 0 ;

PROCESS_THREAD(test, ev, data)
{

	static struct etimer et;

	PROCESS_BEGIN();
		myaddr = init_l2addr_154_char("45:67");

		l2 = startL2_154( myaddr, CHANNEL, PANID); 
		ca = initCasan(l2, MTU, slaveid);

		r1 = initResource (R1_name, R1_title, R1_rt) ;
		setHandlerResource(r1, COAP_CODE_GET, process_temp );
		register_resource(ca, r1);

		r2 = initResource (R2_name, R2_title, R2_rt) ;
		setHandlerResource(r2, COAP_CODE_GET, process_temp );
		register_resource(ca, r2);

		while(1) {    

			if (n % NTAB (resname) == 0)
	    		print_resources (ca) ;
	    	test_resource (ca, l2, resname [n++ % NTAB (resname)]) ;
			
	    	printf("\n");
	    	printf("*************************************************************************");
	    	printf("\n");

	        etimer_set(&et,5*CLOCK_SECOND); 
        	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	    }



   	PROCESS_END();
}