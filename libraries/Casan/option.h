/**
 * @file option.h
 * @brief option class interface
 */

#ifndef CASAN_OPTION_H
#define CASAN_OPTION_H

#include "defs.h"
#include "contiki.h"
#include "stdbool.h" 

#define OPT_ERR_OPTCODE		1
#define OPT_ERR_OPTLEN		2
#define OPT_ERR_RST		0

/**
 * @brief An object of class option represents an individual option
 *
 * This class represents options associated with a message. options are
 * specified by the CoAP protocol.
 *
 * Before sending a message, application must associate options to this
 * message (with the Msg::push_option method).
 * They will be encoded, according to CoAP specification, when
 * the message is about to be sent.
 *
 * Upon message reception, the message will be decoded: options will
 * be created and associated with the received message. Application may
 * retrieve them (with the Msg::reset_next_option and Msg::next_option
 * methods).
 *
 * When an option is created, some points (format, minimum and maximum
 * length) will be checked according to a private table.
 * The format of an option may be:
 * * a string
 * * an unsigned integer
 * * an opaque value
 * * nothing
 *
 * An option opaque value is always internally represented as a byte
 * string, with an added '\0' at the end. This null byte will not be
 * sent on the network, of course, but allows for string manipulation
 * (with functions such as `strlen` or `strcmp` for example) on values
 * returned by option::optval method.
 * Integer options, on another hand, are internally represented as the
 * minimal byte string, according to the CoAP specification.
 * Thus, the value 255 is represented as one byte, whereas 65537 is
 * represented as 3 bytes. This should be completely transparent to
 * applications.
 */



	typedef enum
	{
	    MO_None		= 0,
	    MO_Content_Format	= 12,
	    MO_Etag		= 4,
	    MO_Location_Path	= 8,
	    MO_Location_Query	= 20,
	    MO_Max_Age		= 14,
	    MO_Proxy_Uri	= 35,
	    MO_Proxy_Scheme	= 39,
	    MO_Uri_Host		= 3,
	    MO_Uri_Path		= 11,
	    MO_Uri_Port		= 7,
	    MO_Uri_Query	= 15,
	    MO_Accept		= 16,
	    MO_If_None_Match	= 5,
	    MO_If_Match		= 1,
	    MO_Size1		= 60,
	    MO_Observe		= 6,		// Observe draft
	} optcode_t ;
	typedef unsigned long int uint ;


	typedef enum {
	    cf_none		= -1,		// non-existent option
	    cf_text_plain	= 0,
	} content_format ;

	/** Type needed to minimize diffs to equivalent code of master */
	typedef uint8_t byte ;
	typedef struct option{

		optcode_t optcode_ ;
		int optlen_ ;
		byte *optval_ ;			// 0 if staticval is used
		byte staticval_ [8 + 1] ;	// keep a \0 after, just in case
	} option;

	static uint8_t errno_ ;

	typedef enum
	{
	    OF_NONE		 = 0,
	    OF_OPAQUE,
	    OF_STRING,
	    OF_EMPTY,
	    OF_UINT,
	} optfmt_t ;

	typedef struct optdesc
	{
	    optcode_t code ;
	    optfmt_t format ;
	    int minlen ;
	    int maxlen ;
	} optdesc;
	static optdesc optdesc_ [] ;

	byte uint_to_byte (uint val, int *len) ;

	void freeOption( option *op);

	option *initOption ();

	option *initOptionEmpty (optcode_t optcode);

	option *initOptionOpaque(optcode_t optcode, const void *optval, int optlen);

	option *initOptionInteger (optcode_t optcode, uint optval);

	option *initOptionOption (const option *o);

	void copyOption(option *o1, const option *o2 );

	bool isEqualOption (const option *o1, const option *o2);

	bool isDifferentOption (const option *o1, const option *o2);

	bool isLessThan(const option *o1, const option *o2);

	bool isLessOrEqual(const option *o1, const option *o2);

	bool isGreaterThan(const option *o1, const option *o2);

	bool isGreaterOrEqual(const option *o1, const option *o2);

	optcode_t getOptcode (const option *o);

	void *getOptval (option *o, int *len);

	uint getOptvalInteger (option *o);

	void setOptcode (option *o, optcode_t c);

	void setOptvalOpaque (option *o, void *val, int len);

	void setOptvalInteger (option *o, uint val);

	int getOptlen (const option *o);

	uint8_t get_errno (void);

	void printOption (const option *o);

	void reset_errno (void);

#endif
