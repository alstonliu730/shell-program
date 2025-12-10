#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"
#include "vect_token.h"
#include <assert.h>

// Private Function Declaration
static int _read_string(const char *input, char *copy);
static int isDigit(const char input);
static int isAlpha(const char input);
static int isSymbol(const char* haystack, const char input);

#define TRACKER_LIMIT 64
#define HAYSTACK "_/.-"
/* --------------------------------------------------------------------------------- */
/**
 * Split the given string into tokens.
 *
 * The array of tokens obtained using this function can be freed using 
 * free_vector() on the vector object.
 *
 * @param vec heap-allocated vector data structure
 * @param input input string
 *
 * @return number of tokens found
 */
int get_tokens(vector_t* vec, const char *input) {
    const char* curr = input;

    // Keep track of current token
    char tracker[TRACKER_LIMIT];
    int tok_len = 0;

    // Loop through the input
    while (curr && *(curr) != '\0') {
        // check if it's an alphanumeric char
        if (isAlpha(*curr) || isDigit(*curr) || isSymbol(HAYSTACK, *curr)) {
            tracker[tok_len] = *curr;
            tok_len++;
        } else {
            // check previous token
            if (tok_len > 0) {
                tracker[tok_len] = '\0';

                // add the token
                add_token(vec, tracker, tok_len + 1);

                // reset the tracker
                memset(tracker, 0, TRACKER_LIMIT);
                tok_len = 0;
            }

            // Check symbol table
            switch (*(curr)) {
                case '\"':
                    // read the string
                    tok_len = _read_string((++curr), tracker);
                    curr += (tok_len);

                    // add the token
                    add_token(vec, tracker, tok_len);

                    // reset the tracker
                    memset(tracker, 0, TRACKER_LIMIT);
                    tok_len = 0;
                    break;
                // escape sequence
                case '\\':
                    curr += 1;
                    break;
                // space (ignore)
                case ' ':
                    break;
                // any symbols
                default:
                    add_token(vec, curr, sizeof(char));
                    break;
            }
        }
        
        curr++;
    }

    // check if there's a token left
    if (tok_len > 0) {
        add_token(vec, tracker, tok_len);
    }

    // returns the total number of tokens found
    return vec->size;
}

// Adds a token to the vector
void add_token(vector_t* vec, const char *token, size_t length) {
    assert(vec != NULL);
    add_data(vec, token, length);
}

// Reads in the rest of the string, creates a copy of our string and sets it equal to copy
int _read_string(const char *input, char *copy) {
    unsigned int bytes = 0;
    //While there is input and we have not reached the second quote
    while (*input != '\0') {
        //Set our copy equal to the string and increment the counter
        if (*input == '\"') { break; }
        *copy = *input;
        bytes++;
        copy++;
        input++;
    }
    //Terminate the string
    *copy = '\0';
    bytes++;

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

// Checks if the given character is in the given haystack
int isSymbol(const char* haystack, const char input) {
    assert(&input != NULL);
    return strstr(haystack, &input) != NULL;
}