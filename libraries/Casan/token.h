/**
 * @file token.h
 * @brief token class interface
 */

#ifndef CASAN_TOKEN_H
#define CASAN_TOKEN_H

#include "contiki.h"
#include "defs.h"
#include <stddef.h> 
#include "stdbool.h"

/**
 * @brief An object of class Token represents a token
 *
 * This class represents a token, i.e. a string whose length is
 * limited to 8 bytes.
 */


typedef struct Token {
	int toklen_;
	uint8_t token_[COAP_MAX_TOKLEN]; // no terminating \0 (raw array)
} token;

token *initToken (void);

token *initTokenChar(char *str) ;

token *initTokenToken(uint8_t *val, size_t len);

bool isEqualToken( const token t1, const token t2);

bool isDifferentToken( const token t1, const token t2);

void resetToken(token *to);

void printToken(token *to) ;


#endif
