#ifndef _VECTOR_H
#define _VECTOR_H

#include <stddef.h>

//Declare vector structure
typedef struct {
    int size;
    int capacity;
    char **data; 
} vector_t;

/**
 * Initializes the vector given the address.
 * 
 * Vector address will need to be freed using free_vector() method.
 * 
 * @param capacity the number of space inside the vector
 * 
 * @return address to the vector object
 */
vector_t* vect_init(int capacity);

/**
 * Reallocates more memory for the vector data field.
 * Resizes by a factor of VECT_MAX.
 * 
 * @param vect address to the vector object
 */
void vect_grow(vector_t* vec);

/**
 * Returns if the vector is full.
 * 
 * @param vect address to the vector object
 * 
 * @return Returns `1` if the vector is full, otherwise `0`.
 */
int vect_isFull(vector_t* vec);

/**
 * Returns if the vector is empty.
 * 
 * @param vect address to the vector object
 * 
 * @return Returns `1` if the vector is empty, otherwise `0`.
 */
int vect_isFull(vector_t* vec);

/**
 * Adds the given input with the length of the input.
 * 
 * This allocates memory from the heap.
 * 
 * @param vec address to the vector object
 * @param input string input
 * @param length size of the string input
 */
void add_data(vector_t* vec, const char* input, size_t length);

/**
 * Free the memory in the vector including the data array.
 * 
 * Memory needs to be allocated via malloc.
 * 
 * @param vect address to the vector object
 */
void free_vector(vector_t* vec);

#endif /* */