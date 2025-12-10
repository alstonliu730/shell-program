#include "vector.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

// initialize the vector object and the 
vector_t* vect_init(int capacity) {
    // allocate memory for vector object
    vector_t* vec = (vector_t *) malloc(sizeof(vector_t));
    assert(vec != NULL);
    
    // allocate memory for the list of strings
    // NOTE: need to allocate memory for each address
    vec->data = (char **) malloc(sizeof(char *) * capacity);
    assert(vec->data != NULL);

    // Initialize vector size and capacity
    vec->size = 0;
    vec->capacity = capacity;
    
    // Return the address of the vector
    return vec;
}

// Grow our vector when necessary
void vect_grow(vector_t* vec) {
    assert(vec != NULL);
    // calculate the new size
    size_t new_size = vec->capacity * 2;

    // reallocate the memory
    char** new_p = (char **) realloc(vec->data, new_size);

    if (new_p != NULL) {
        vec->data = new_p;
    } else {
        // error when allocating memory
        free_vector(vec);
        perror("Reallocation failed for vector.");
        exit(EXIT_FAILURE);
    }
}

// adds data into the vector object and returns the number of bytes written
void add_data(vector_t* vec, const char* input, size_t length) {
    assert(vec != NULL);

    // allocate memory for the char array
    if (vec->size + 1 >= vec->capacity) {
        // allocate more space
        vect_grow(vec);
    }

    // allocate memory for the new token
    char* new_token = (char *) malloc(sizeof(char) * length);
    strncpy(new_token, input, length);

    // add to the vector
    *(vec->data + vec->size) = new_token;
    vec->size++;
}

//Returns if our vector is at capacity
int vect_isFull(vector_t* vec) {
    assert(vec != NULL);
    return (vec->size == vec->capacity);
}

// Returns if the vector is empty
int vect_isEmpty(vector_t* vec) {
    assert(vec != NULL);
    return (vec->size == 0);
}

// Frees the memory inside the vector struct
void free_vector(vector_t* vec) {
    assert(vec != NULL);
    
    // Free the data inside the list of strings
    for(int i = 0; i < vec->size; i++) {
        free(*(vec->data+i));
    }

    // Free the array of data
    free(vec->data);

    // Free the vector object
    free(vec);
}