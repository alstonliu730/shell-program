#include <stdio.h>
#include "vect_token.h"
#include "vector.h"
#include <assert.h>


//Main function
int main(int argc, char **argv) {
  //Create a variable to take in stdin
  char input[256];

  fgets(input, 256, stdin);

  //Create a vector to tokenize our input
  vector_t* token_vect = get_tokens(input);

  char **tokens = token_vect->data;

  //Reference current to tokens vector and print each token out
  char **current = tokens;
  while (*current != NULL) {
    printf("%s\n", *current);
    ++current;
  }

  //Free our tokens vector 
  free_tokens(token_vect);
  return 0;
}
