/**
 * @file token.cpp
 * @brief Token class implementation
 */

#include "token.h"

/******************************************************************************
 * Constructors and destructors
 */


void freeToken(token *to) {
    free(to);
}

/**
 * Default constructor
 *
 * The default constructor initializes option attributes
 */

token *initToken(void)
{
    token *to = (token *) malloc (sizeof (struct Token));
    to->toklen_ = 0 ;
    return to;
}



/**
 * Constructor with initialization string (terminated with \0)
 */

token *initTokenChar(char *str) {
 	token *to = (token *) malloc (sizeof (struct Token));
 	int i =0;

 	while (str [i] != 0 && i < NTAB (to->token_))
    {
    	to->token_[i] = str[i];
    	i++;
    }

    to->toklen_ = i ;
    return to;
}



/**
 * Constructor with an existing token
 *
 * This constructor copies an existing token into the current object
 *
 * @param val token value
 * @param len token length
 */

token *initTokenToken(uint8_t *val, size_t len) {
 	token *to = (token *) malloc (sizeof (struct Token));
 	if (len > 0 && len < NTAB (to->token_)) {
 		to->toklen_ = len;
 		memcpy( to->token_, val, len);
 	} else to->toklen_ = 0;
 	return to;
}


 /******************************************************************************
 * Operators
 */

bool isEqualToken( const token t1, const token t2) {
 	return t1.toklen_ == t2.toklen_ && memcmp(t1.token_, t2.token_, t1.toklen_) == 0;
}
 
bool isDifferentToken( const token t1, const token t2) {
 	return t1.toklen_ != t2.toklen_ || memcmp(t1.token_, t2.token_, t1.toklen_) != 0;
}



 /******************************************************************************
 * Commodity functions
 */

void resetToken(token *to){
 	to->toklen_ = 0;
}

void printToken(token *to) {
	int i;

	for (i = 0; i< to->toklen_ ; i++) {
	printf("%x\n ",to->token_[i] );
	}
}