#include <stdio.h>
#include "vect_token.h"
#include "vector.h"
#include <assert.h>
#include <stdint.h>

//Main function
int main(int argc, char **argv) {
  //Create a variable to take in stdin
  char input[256];

  //Read a line from the standard input stream
  fgets(input, 256, stdin);

  //Create a vector to tokenize our input
  vector_t* token_vect = vect_init(256);
  
  // get tokens from the input
  get_tokens(token_vect, input);

  //Reference current to tokens vector and print each token out
  char **current = token_vect->data;;
  while (*current != NULL) {
    printf("%s\n", *current);
    ++current;
  }

  // Free our tokens vector 
  free_vector(token_vect);
  return 0;
}
