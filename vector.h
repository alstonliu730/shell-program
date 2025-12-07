#ifndef _VECTOR_H
#define _VECTOR_H

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
 * @param vect address to the vector object
 */
void vect_init(vector_t* vec, int capacity);

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
 * @return Returns `True` if the vector is full, otherwise `False`.
 */
int vect_isFull(vector_t* vec);

/**
 * Returns if the vector is empty.
 * 
 * @param vect address to the vector object
 * 
 * @return Returns `True` if the vector is empty, otherwise `False`.
 */
int vect_isFull(vector_t* vec);

/**
 * Free the memory in the vector including the data array.
 * 
 * Memory needs to be allocated via malloc.
 * 
 * @param vect address to the vector object
 */
void free_vector(vector_t* vec);

#endif /* */