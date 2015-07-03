/**
 * @file option.cpp
 * @brief option class implementation
 */

#include "option.h"


static uint8_t errno_ = 0 ;


#define RESET(op)       do {                    \
                op->optcode_ = MO_None ;        \
                op->optlen_ = 0 ;           \
                op->optval_ = 0 ;           \
            } while (false)             // no " ;"
#define COPY_VAL(op,p) do {                    \
                byte *b ;               \
                if (op->optlen_ + 1 > (int) sizeof op->staticval_) { \
                op->optval_ = (uint8_t*) malloc (op->optlen_+ 1) ; \
                b = op->optval_ ;           \
                }                   \
                else                \
                {                   \
                op->optval_ = 0 ;           \
                b = op->staticval_ ;        \
                }                   \
                memcpy (b, (p), op->optlen_) ;      \
                b [op->optlen_] = 0 ;           \
            } while (false)             // no " ;"
#define CHK_OPTCODE(c,err) do {                 \
                int i ;             \
                (err) = true ;          \
                for (i = 0 ; i < NTAB (optdesc_) ; i++) \
                {                   \
                if (optdesc_ [i].code == c) \
                {               \
                    (err) = false ;     \
                    break ;         \
                }               \
                }                   \
            } while (false)             // no " ;"
#define CHK_OPTLEN(c,l,err) do {                \
                int i ;             \
                (err) = true ;          \
                for (i = 0 ; i < NTAB (optdesc_) ; i++) \
                {                   \
                if (optdesc_ [i].code == c  \
                    && (l) >= optdesc_ [i].minlen   \
                    && (l) <= optdesc_ [i].maxlen) \
                {               \
                    (err) = false ;     \
                    break ;         \
                }               \
                }                   \
            } while (false)             // no " ;"


static optdesc optdesc_ [] =
{
    { MO_Content_Format,	OF_OPAQUE,	0, 8	},
    { MO_Etag,			OF_OPAQUE,	1, 8	},
    { MO_Location_Path,		OF_STRING,	0, 255	},
    { MO_Location_Query,	OF_STRING,	0, 255	},
    { MO_Max_Age,		OF_UINT,	0, 4	},
    { MO_Proxy_Uri,		OF_STRING,	1, 1034	},
    { MO_Proxy_Scheme,		OF_STRING,	1, 255	},
    { MO_Uri_Host,		OF_STRING,	1, 255	},
    { MO_Uri_Path,		OF_STRING,	0, 255	},
    { MO_Uri_Port,		OF_UINT,	0, 2	},
    { MO_Uri_Query,		OF_STRING,	0, 255	},
    { MO_Accept,		OF_UINT,	0, 2	},
    { MO_If_None_Match,		OF_EMPTY,	0, 0	},
    { MO_If_Match,		OF_OPAQUE,	0, 8	},
    { MO_Observe,		OF_UINT,	0, 3	}
} ;


/******************************************************************************
 * Utilities
 */

static byte *uint_to_byte (uint val, int *len) {

    static byte stbin [sizeof (uint)] ;
    int shft ;

    // translate in network byte order, without leading null bytes
    *len = 0 ;
    for (shft = sizeof val - 1 ; shft >= 0 ; shft--)
    {
        byte b ;

        b = (val >> (shft * 8)) & 0xff ;
        if (len != 0 || b != 0)
            stbin [*len++] = b ;
    }
    return stbin ;
}


//free option
void freeOption( option *op) {
    free(op->optval_);
    free(op);
}


/**
 * Default constructor
 *
 * The default constructor initializes option attributes
 */

option *initOption ()
{
    option *op = (option *)malloc (sizeof(struct option));
    RESET(op) ;
    return op;
}

/**
 * Constructor for an empty option
 *
 * This constructor creates an option without any value, which can be
 * initialized later with the Option::optval method.
 *
 * @param optcode the option code
 */

option *initOptionEmpty (optcode_t optcode) {
    option *op = (option *) malloc (sizeof(struct option));
    bool err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err) {
        printf("option::optval err: CHK_OPTCODE 1\n" );
        errno_ = OPT_ERR_OPTCODE ;
    }
    RESET(op);
    op->optcode_ = optcode;
    return op;
}


/**
 * Constructor for an option with an opaque value
 *
 * This constructor creates an option with an opaque format value.
 * The value itself is copied in the option.
 *
 * @param optcode the option code
 * @param optval pointer to value
 * @param optlen length of value
 */

 option *initOptionOpaque(optcode_t optcode, const void *optval, int optlen) {
    option *op = (option *)malloc (sizeof(struct option));
    bool err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err) {
        printf("option::optval err: CHK_OPTCODE 2") ;
        errno_ = OPT_ERR_OPTCODE ;
    }
    CHK_OPTLEN (optcode, optlen, err) ;
    if (err) {
        printf("option::optval err: CHK_OPTLEN 2") ;
        errno_ = OPT_ERR_OPTLEN ;
    }
    RESET(op) ;
    op->optcode_ = optcode ;
    op->optlen_ = optlen ;
    COPY_VAL(op, optval);
    return op;
 }


/**
 * Constructor for an option with an integer value
 *
 * This constructor creates an option with an integer value. This
 * value will be converted (packed) in the minimal string of bytes
 * according to the CoAP specification.
 *
 * @param optcode the option code
 * @param optval integer value
 */

option *initOptionInteger (optcode_t optcode, uint optval)
{
    option *op = (option *)malloc (sizeof(struct option));
    bool err ;
    byte *stbin ; 
    int len;

    stbin = uint_to_byte (optval, &len) ;
    err = false ;
    CHK_OPTCODE (optcode, err) ;
    if (err) {
        printf("option::optval err: CHK_OPTCODE 3\n") ;
        errno_ = OPT_ERR_OPTCODE ;
    }
    CHK_OPTLEN (optcode, len, err) ;
    if (err) {
        printf ("option::optval err: CHK_OPTLEN 3\n") ;
        errno_ = OPT_ERR_OPTLEN ;
    }       
    RESET(op) ;
    op->optcode_ = optcode ;
    op->optlen_ = len ;
    COPY_VAL (op,stbin) ;
    return op;
}



/**
 * Copy constructor
 *
 * This constructor copies an existing option
 */

option *initOptionOption (const option *o)
{
    option *op= NULL;
    memcpy (op, &o, sizeof o) ;
    if (op->optval_)
        COPY_VAL (op,o->optval_) ;
    return op;
}


/**
 * Copy assignment constructor
 *
 * This constructor copies an existing option
 */

void copyOption(option *o1, const option *o2 ){
    if (isDifferentOption(o1, o2)) {
        if(o1->optval_) {
            free(o1->optval_);
            o1->optval_ = NULL;
        }

        memcpy(o1, o2, sizeof *o1);
        if (o2->optval_) 
            COPY_VAL(o1, o2->optval_);
    }
}


/******************************************************************************
 * Operator used for list sorting (cf msg.cc)
 */


bool isEqualOption (const option *o1, const option *o2)
{
    return o1->optcode_ == o2->optcode_ ;
}

bool isDifferentOption (const option *o1, const option *o2)
{
    return o1->optcode_ != o2->optcode_ ;
}


bool isLessThan(const option *o1, const option *o2)
{
    return o1->optcode_ < o2->optcode_ ;
}

bool isLessOrEqual(const option *o1, const option *o2)
{
    return o1->optcode_ <= o2->optcode_ ;
}

bool isGreaterThan(const option *o1, const option *o2)
{
    return o1->optcode_ > o2->optcode_ ;
}

bool isGreaterOrEqual(const option *o1, const option *o2)
{
    return o1->optcode_ >= o2->optcode_ ;
}


/******************************************************************************
 * Accessors
 */

 /**
 * Get the option code
 */

optcode_t getOptcode (const option *o)
{
    return o->optcode_ ;
}


/**
 * Get the option value and length
 *
 * @param len address of an integer which will contain the length
 *  in return, or null address if length is not wanted
 * @return pointer to the option value (do not free this address)
 */

void *getOptval (option *o, int *len)
{
    if (len != (int *) 0)
        *len = o->optlen_ ;
    return (o->optval_ == 0) ? o->staticval_ : o->optval_ ;
}


/**
 * Get the option value as an integer
 *
 * The option value is unpacked as an integer.
 *
 * @return option value as a standard (unsigned) integer
 */

uint getOptvalInteger (option *o)
{
    uint v ;
    byte *b ;
    int i ;

    v = 0 ;
    b = (o->optval_ == 0) ? o->staticval_ : o->optval_ ;
    for (i = 0 ; i < o->optlen_ ; i++)
        v = (v << 8) & b [i] ;
    return v ;
}


/******************************************************************************
 * Mutators
 */


/**
* Assign the option code
*/

void setOptcode (option *o, optcode_t c)
{
    o->optcode_ = c ;
}

/**
 * Assign an opaque value to the option
 *
 * @param val pointer to the value to be copied in the option
 * @param len length of value
 */

void setOptvalOpaque (option *o, void *val, int len)
{
    o->optlen_ = len ;
    COPY_VAL (o,val) ;
}


/*
 * Assign an integer value to the option
 *
 * The value will be converted (packed) in the minimal string of bytes
 * according to the CoAP specification.
 *
 * @param val integer to copy in option
 */

void setOptvalInteger (option *o, uint val)
{
    bool err ;
    byte *stbin ;
    int len ;

    stbin = uint_to_byte (val, &len) ;
    err = false ;
    CHK_OPTLEN (o->optcode_, len, err) ;
    if (err)
    {
        printf("option::optval err: CHK_OPTLEN\n") ;
        errno_ = OPT_ERR_OPTLEN ;
        return ;
    }
    o->optlen_ = len ;
    COPY_VAL (o, stbin) ;
}


/**
 * Option length
 *
 * The option length does not include the added null byte used internally.
 *
 * @return length of option value
 */

int getOptlen (const option *o)
{
    return o->optlen_ ;
}


/**
 * Returns the last error encountered during an option assignment
 */

uint8_t get_errno (void)
{
    return errno_ ;
}

void printOption (const option *o)
{
    printf ("%s : %s=", YELLOW ("OPTION"), RED ("optcode")) ;
    switch ((unsigned char) o->optcode_)
    {
    case MO_None        : printf("MO_None") ; break ;
    case MO_Content_Format  : printf("MO_Content_Format") ; break;
    case MO_Etag        : printf("MO_Etag") ; break ;
    case MO_Location_Path   : printf("MO_Location_Path") ; break ;
    case MO_Location_Query  : printf("MO_Location_Query") ; break;
    case MO_Max_Age     : printf("MO_Max_Age") ; break ;
    case MO_Proxy_Uri   : printf("MO_Proxy_Uri") ; break ;
    case MO_Proxy_Scheme    : printf("MO_Proxy_Scheme") ; break ;
    case MO_Uri_Host    : printf("MO_Uri_Host") ; break ;
    case MO_Uri_Path    : printf("MO_Uri_Path") ; break ;
    case MO_Uri_Port    : printf("MO_Uri_Port") ; break ;
    case MO_Uri_Query   : printf("MO_Uri_Query") ; break ;
    case MO_Accept      : printf("MO_Accept") ; break ;
    case MO_If_None_Match   : printf("MO_If_None_Match") ; break ;
    case MO_If_Match    : printf("MO_If_Match") ; break ;
    default :
        printf ("%s", RED ("ERROR")) ;
        printf("%d", (unsigned char) o->optcode_) ;
        break ;
    }
    printf ("/") ;
    printf (o->optcode_) ;
    printf ("%s=",BLUE (" optlen")) ;
    printf (o->optlen_) ;

    if (o->optval_)
    {
    printf ("%s=",BLUE (" optval") ) ;
    char *buf = (char *) malloc (sizeof (char) * o->optlen_ + 1) ;
    memcpy (buf, o->optval_, o->optlen_) ;
    buf[o->optlen_] = '\0' ;
    printf(buf) ;
    free (buf) ;
    }
    else if (o->optlen_ > 0 )
    {
    printf ("%s=",BLUE (" staticval") ) ;
    char *buf = (char *) malloc (sizeof (char) * o->optlen_ + 1) ;
    memcpy (buf, o->staticval_, o->optlen_) ;
    buf[o->optlen_] = '\0' ;
    printf (buf) ;
    free (buf) ;
    }
    printf("\n") ;
}


void reset_errno (void)
{
    errno_ = 0 ;
}