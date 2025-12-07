#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "vect_token.h"
#include <assert.h>

static vector_t* token_vect = NULL;

static int read_string(const char *input, char *copy);
static int isDigit(const char input);
static int isAlpha(const char input);
static int isSymbo(const char input);
static const char escape_lookup[] = {
    ['t'] = '\t',
    ['n'] = '\n',
    ['r'] = '\r',
    ['v'] = '\v',
    ['f'] = '\f'
};

/* --------------------------------------------------------------------------------- */
/**
 * Split the given string into tokens.
 *
 * The array of tokens obtained using this function can be freed using 
 * free_tokens().
 *
 * @param input input string
 *
 * @return a null-terminated heap-allocated array of token strings
 */
vector_t* get_tokens(const char *input) {
    // initialize the vector object
    vect_init(token_vect, VECT_MAX);

    char* curr = input;
    // Loop through the input
    while (curr && curr != '\0') {
        switch (curr) {
            case '\\':
                if ((curr + 1)) {
                    escape_lookup[(++curr)];
                }
        }
        curr++;
    }
}

//Adds a token assuming our vector is not at capacity
void add_token(vector_t* vec, const char *token, size_t length) {
    assert(vec != NULL);

    // Check if the vector has space
    if (vect_isFull(vec)) {
        // allocate more space
        vect_grow(vec);
    }

    // allocate memory for the new token
    char* new_tok = (char *) malloc(sizeof(char) * length);
    strncpy(new_tok, token, length);

    //
}

// Reads in the rest of the string, creates a copy of our string and sets it equal to copy
int read_string(const char *input, char *copy) {
    unsigned int bytes = 0;

    //While there is input and we have not reached the second quote
    while (*input && *input != '"') {
      //Set our copy equal to the string and increment the counter
      *copy = *input;
      ++bytes;
      ++copy;
      ++input;
    }
    //Terminate the string
    *copy = '\0';

    //Return the size to iterate over the quoted string
    return bytes;
}

// Checks if the given character is a digit 
int isDigit(const char input) {
    return (input >= '0' && input <= '9');
}

// Checks if the given character is an alphabet
int isAlpha(const char input) {
    return ((input >= 'a') && (input <= 'z')) ||
        ((input >= 'A') && (input <= 'Z'));
}

