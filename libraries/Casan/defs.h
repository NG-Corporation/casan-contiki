#ifndef __DEFS_H__
#define __DEFS_H__
/*
 * common definitions
 *
 */


#define NTAB(t)		((int) (sizeof (t)/sizeof (t)[0]))


#define	BYTE_HIGH(n)		(((n) & 0xff00) >> 8)
#define	BYTE_LOW(n)		((n) & 0xff)

#define C_CSI		"\033["
#define	C_RESET		C_CSI "m"

// normal colors
#define	C_BLACK		C_CSI "30m"
#define	C_RED		C_CSI "31m"
#define	C_GREEN		C_CSI "32m"
#define	C_YELLOW	C_CSI "33m"
#define	C_BLUE		C_CSI "34m"
#define	C_MAGENTA	C_CSI "35m"
#define	C_CYAN		C_CSI "36m"
#define	C_WHITE		C_CSI "37m"

// bright colors
#define	B_BLACK		C_CSI "30;1m"
#define	B_RED		C_CSI "31;1m"
#define	B_GREEN		C_CSI "32;1m"
#define	B_YELLOW	C_CSI "33;1m"
#define	B_BLUE		C_CSI "34;1m"
#define	B_MAGENTA	C_CSI "35;1m"
#define	B_CYAN		C_CSI "36;1m"
#define	B_WHITE		C_CSI "37;1m"

#define	RED(m)			B_RED m C_RESET
#define	BLUE(m)			C_BLUE m C_RESET
#define	YELLOW(m)		C_YELLOW m C_RESET

 
// Maximum token length
#define	COAP_MAX_TOKLEN		8

#endif