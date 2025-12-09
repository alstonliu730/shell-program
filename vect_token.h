#ifndef _VECT_TOKEN_H
#define _VECT_TOKEN_H

#include "vector.h"

//Define max vector capactiy
#define VECT_MAX 256

/**
 * Split the given string into tokens.
 *
 * The array of tokens obtained using this function can be freed using 
 * free_tokens().
 *
 * @param vec address to the vector object
 * @param input input string
 *
 * @return number of tokens found
 */
int get_tokens(vector_t* vec, const char *input);

/**
 * Free the given array of tokens created using get_tokens().
 *
 * Both the array and each of the non-null pointers need to be allocated
 * using malloc.
 *
 * @param vec heap-allocated vector objects with the tokens.
 */
void free_tokens(vector_t* vec);

/**
 * Adds a token to the vector data structure.
 * 
 * @param vec address to the vector object
 * @param token token string to input
 * @param length number of characters in the token
 */
void add_token(vector_t* vec, const char *token, size_t length);

#endif /* _TOKENS_H */
