#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include "vector.h"
#include "vect_token.h"

// max number of input per line
#define MAX_INPUT 256
#define HALF_INPUT (MAX_INPUT / 2)
#define QUARTER_INPUT (MAX_INPUT / 4)

// max number of command sequence
#define MAX_SEQ 8

// Function declarations
void parse_exec(char* input, int in_fd, int out_fd);
void nullify(char **args);
void execute(vector_t** cmd_vect, int num_commands);
void free_cmd_vect(vector_t** cmd_vect);

// parses through the input string to split into different argument sequences
void parse_exec(char* input, int in_fd, int out_fd) {
    assert(input != NULL);

    vector_t* tok_vect = vect_init(MAX_INPUT);  // Create a local vector for the tokens
    int num_tok = get_tokens(tok_vect, input);  // get tokens from the input
    char **curr_tok = tok_vect->data;           // create a pointer to traverse through tokens
    
    // create a child process
    pid_t child = fork();
    if (child == 0) { // in child process
        int pipe_fd[2];
        // traverse through the tokens
        for(int i = 0; i < num_tok; i++) {
            // check for output redirection and next argument
            if (strcmp(curr_tok[i], ">") == 0 && (i + 1) < tok_vect->size && strlen(curr_tok[i+1]) > 0) {
                // close stdout
                close(out_fd);

                // open file and replace stdout with new file descriptor
                int fd = open(curr_tok[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                assert(fd == out_fd);
            }

            // check for input redirection and next argument
            if (strcmp(curr_tok[i], "<") == 0 && (i + 1) < tok_vect->size && strlen(curr_tok[i+1]) > 0) {
                // close stdin
                close(in_fd);

                // open file and replace stdin with new file
                int fd = open(curr_tok[i], O_RDONLY);
                assert(fd == in_fd);
            }
            
            // check if the current argument is a pipe
            if (strcmp(curr_tok[i], "|") == 0) {
                // create the pipe_fd
                assert(pipe(pipe_fd) != -1);

                // fork a child and execute the next command
                // parse_exec([cmd2, arg, arg ..., pipe_fd[0], outfd)
            }
        }
    } else if (child < 0) { // error in fork
        perror("Fork() failed");

        // Free the token vector
        free_vector(tok_vect);
        exit(EXIT_FAILURE);
    } else { // in parent process
        // wait for the child to finish its process
        wait(NULL);

        // free the token vector
        free_vector(tok_vect);
    }    
}

// clears the given argument string array
void nullify(char **args) {
    for (int i = 0; i < MAX_INPUT; i++) {
        memset(args[i], 0, sizeof(char*));
    }
}

// prepares a call to exec_pipe using the given arg string array
void make_pipe(char** args) {

}

// execute a piping command
void exec_pipe(char** left, char** right) {
} 

// prepares a call to redirect using the given arg string array
void make_redirect(char** args) {

}


// performs a redirect on the given args and file in the given dir
void redirect(char** args, char* filename, int dir) {

}

// returns the length of a given arg string array
int arglen(char** args) {
    char** itr = args;
    int len = 0;
    while (*itr != NULL) {
        len++;
        ++itr;
    }
    return len;
}

// executes commands in the arguments
void execute(vector_t** cmd_vect, int num_commands) {
    assert(cmd_vect != NULL);

    // go through each command
    for (int i = 0; i < num_commands; i++) {
        if (prepare_exec(cmd_vect[i], STDIN_FILENO, STDOUT_FILENO) != 0) { // error detected
            // free the command vector
            free_cmd_vect(cmd_vect);

            // exit out of the program safely
            exit(EXIT_FAILURE);
        }
    }
}

// frees the memory inside of the command vector
void free_cmd_vect(vector_t** cmd_vect) {
    for(int i = 0; i < MAX_SEQ; i++) {
      free_vector(cmd_vect[i]); // free each vector
    }

    // free the pointer itself
    free(cmd_vect);
}

//main method for user input to mini shell
int main(int argc, char **argv) {
    // welcome message
    printf("Welcome to mini-shell.\n");

    // keep running the shell program indefinitely
    while(1) {
      printf("shell $ ");

      // place to store input from user
      char input[MAX_INPUT];
      
      // read user input from stdin
      if (fgets(input, MAX_INPUT, stdin) == NULL) { // (CTRL-D)
        // end program
        printf("Bye bye.\n");
        return 0;
      }

      // parse input into command sequences
      parse_exec(input, STDIN_FILENO, STDOUT_FILENO);
    }

    // should not return here (since user will exit through a command)
    perror("Undefined behavior detected.");
    return 1;
}
