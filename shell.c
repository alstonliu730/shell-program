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

// DEBUGGING PRINT
#ifdef DEBUG
    #define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__);
#else
    #define DEBUG_PRINT(...) do {} while(0);
#endif

// max number of input per line
#define MAX_INPUT 256
#define HALF_INPUT (MAX_INPUT / 2)
#define QUARTER_INPUT (MAX_INPUT / 4)

// Child Error Status
#define ERROR_SUCCESS 0x1
#define ERROR_EXIT 0xF
#define ERROR_FAIL 0x10

// max number of command sequence
#define MAX_SEQ 8

// symbols for scanner
const char haystack[] = "()|<>";

// Function declarations
char** parse(char* input, int* size);
void execute(int argc, char** argv);
static void _exec_cmd(char** argv, int* err_no);
static void _handle_parenthesis(int argc, char** argv);
static int _find_operator(int argc, char** argv);
static void free_strings(char** arr, int n);

/**
 * Parses through the input and returns the list of tokens.
 * 
 * @param input user commands in a string format
 * @param size resulting number of tokens found
 * @return address to the tokens
 */
char** parse(char* input, int* size) {
    assert(input != NULL);

    DEBUG_PRINT("Parse input: %s\n", input);
    vector_t* tok_vect = vect_init(MAX_INPUT);  // Create a local vector for the tokens
    int num_tok = get_tokens(tok_vect, input);  // get tokens from the input
    
    // allocate memory for the output buffer
    char** tokens = (char **) malloc(sizeof(char*) * num_tok);

    // copy tokens into the new buffer
    for(int i = 0; i < num_tok; i++) {
        // local variables
        size_t length = strlen(tok_vect->data[i]) + 1; // include null-terminated char
        char* curr = tok_vect->data[i];

        // allocate memory for each string
        tokens[i] = (char *) malloc(length * sizeof(char));

        // copy string into result array
        strncpy(tokens[i], curr, length);    
    }

    // free the token vector
    free_vector(tok_vect);

    // return the resulting array and number of tokens
    *size = num_tok;
    return tokens;
}

/**
 * Returns the first occurance of the lowest-priority operator in the given command line.
 * - ";" (lowest)
 * - "|"
 * - "<"
 * - ">" (highest)
 * - "()" (ignored)
 * 
 * For parenthesis, we will use a depth variable to track and ignore other operators
 * in the statement until the next parenthesis.
 * 
 * @param argc the number of tokens
 * @param argv the list of tokens 
 * 
 * @returns the position of the lowest-priority operator
 * 
 */
static int _find_operator(int argc, char** argv) {
    int depth = 0;
    int semicolon_pos = -1, pipe_pos = -1, redir_pos = -1;

    // Traverse through commands
    for(int i = 0; i < argc; i++) {
        // ignore non symbol inputs
        if (strlen(argv[i]) != 1) {
            continue;
        }
        
        // get symbol character
        char operator = argv[i][0];

        // ignore all parentheses
        if (operator == '(') {
            depth++;
            continue;
        }

        if (operator == ')') {
            depth--;
            continue;
        }

        // no parenthesis
        if (depth == 0) {
            // return first occurance of semicolon (lowest priority)
            if (operator == ';' && semicolon_pos == -1) {
                return i;
            }
            // keep first occurance of pipe
            else if (operator == '|' && pipe_pos == -1) {
                pipe_pos = i;
            } 
            
            // keep first occurance of redirection symbol
            else if ((operator == '<' || operator == '>') && redir_pos == -1) {
                redir_pos = i;
            }
        }
    }

    // Prioritize operators
    return (pipe_pos != 0) ? pipe_pos : ((redir_pos != 0) ? redir_pos : -1);
}

// executes commands in the arguments (divide and conquer)
void execute(int argc, char** argv) {
    assert(argv != NULL);
    assert(argc >= 1);
    
    #ifdef DEBUG
        DEBUG_PRINT("DEBUG: execute: argv = \n")
        for (int i = 0; i < argc; i++) {
            DEBUG_PRINT("- argv[%d] = %s\n", i, argv[i])
        }
    #endif

    // find the pivot until base case
    int pivot = _find_operator(argc, argv);
    char** left;
    char** right;

    // Base Case
    if (pivot < 0 || pivot == (argc - 1)) {
        // Check for parentheses
        if (argv[0][0] == '(' && argv[argc - 1][0] == ')') {
            _handle_parenthesis(argc, argv);
            return;
        } 

        // Fork a child and execute the command
        pid_t child = fork();
        
        if (child == 0) {           // CHILD
            int err = 0; // status

            // use execute helper function
            _exec_cmd(argv, &err);
        } else {                    // PARENT
            int status;
            waitpid(child, &status, 0);

            // Return from the child
            status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            DEBUG_PRINT("Status: %d\n", status);

            // check error status 
            switch (status) {
                case ERROR_SUCCESS:
                    DEBUG_PRINT("Executed Successfully.\n")
                    break;

                case ERROR_EXIT: // exit function
                {
                    DEBUG_PRINT("Exiting Shell with status %d\n", WEXITSTATUS(status))
                    // Free the memory from argv
                    free_strings(argv, argc);

                    // Exit the entire program
                    printf("Bye Bye.\n");
                    exit(0);
                    break;
                }
                default: // error 
                    break;
            }
			return;
        }
    } else {
		// Check operator
		char operator = argv[pivot][0];
		
		int nLeft = pivot; // 0 -> (pivot - 1) (inclu.)
		int nRight = argc - pivot - 1; // (pivot + 1) -> (argc - 1) (inclu.)
		switch (operator) {
			case ';':
				// split the statement into left and right commands
				left = (char**) malloc(sizeof(char*) * nLeft);
				right = (char**) malloc(sizeof(char*) * nRight);

				// copy the left statements
				for(int i = 0; i < nLeft; i++) {
					int tok_len = strlen(argv[i]) + 1;
					left[i] = (char *) malloc(sizeof(char) * tok_len);

					strncpy(left[i], argv[i], tok_len);
				}

				// copy the right statements
				for (int j = pivot + 1; j < argc; j++) {
					int tok_len = strlen(argv[j]) + 1;
					right[j - pivot - 1] = (char *) malloc(sizeof(char) * tok_len);

					strncpy(right[j - pivot - 1], argv[j], tok_len);
				}

				// execute first left then right
				execute(nLeft, left);
				execute(nRight, right);

				// free the memory for both statements
				free_strings(left, nLeft);
				free_strings(right, nRight);
				break;
			default:
				break;
		}
		return;
    }
}

// Command helper (Assumes this is a child)
static void _exec_cmd(char** argv, int* err_no) {
    char* command = argv[0];
    DEBUG_PRINT("_exec_cmd: command = \"%s\"\n", command);

    // Built-in commands
    if(strncmp(command, "exit", strlen("exit")) == 0) {
        DEBUG_PRINT("PID %d exiting with status %d\n", getpid(), ERROR_EXIT)
        _exit(ERROR_EXIT);
    } else {    // Otherwise 
        execvp(argv[0], argv); // replaces the process
    }
    
    // error
    fprintf(stderr, "Unknown Command: %s\n", command);
    _exit(ERROR_FAIL);
}


// Assuming that the input is a full parenthesis statement
static void _handle_parenthesis(int argc, char** argv) {
    // new length for the statement
    int new_length = argc - 2;

    // allocate memory for the statement inside
    char **str_cmds = (char **) malloc(new_length * sizeof(char*));

    // copy the statement inside 
    for(int i = 1; i < (argc - 1); i++) {
        int tok_len = strlen(argv[i]);
        str_cmds[i - 1] = (char *) malloc(tok_len * sizeof(char));

        // copy argument into new string array
        memcpy(str_cmds[i - 1], argv[i], tok_len);
    }

    // execute the command
    execute(new_length, str_cmds);

    // free the memory
    free_strings(str_cmds, new_length);
    return;
}

// free the strings in the allocated array
static void free_strings(char** arr, int n) {
	for(int i = 0; i < n; i++) {
		free(arr[n]);
	}
	free(arr);
}

// main method for user input to mini shell
int main(int argc, char **argv) {
    // welcome message
    printf("Welcome to mini-shell.\n");

    // keep running the shell program indefinitely
    while(1) {
        printf("shell $ ");

        // place to store input from user
        char input[MAX_INPUT];
        fflush(stdin);

        // read user input from stdin
        size_t len;
        if (fgets(input, MAX_INPUT, stdin) != NULL) {
            len = strlen(input);
            if (len > 0 && input[len - 1] == '\n') {
                // replace the newline with null terminated string
                input[len - 1] = '\0';
            }
            DEBUG_PRINT("DEBUG: User entered %s\n", input)
        } else {        // (CTRL-D)
            // end program
            printf("Bye bye.\n");
            exit(0);
        }

        // parse input into token
        int nToken = 0;
        char** tokens = parse(input, &nToken);

        // If there's no input continue
        if (nToken == 0) {
            continue;
        }

        #ifdef DEBUG
            DEBUG_PRINT("DEBUG tokens: \n")
            for (int i = 0; i < nToken; i++) {
                DEBUG_PRINT("- tokens[%d] = %s\n", i, tokens[i])
            }
        #endif

        // execute the commands
        execute(nToken, tokens);

        // Free the memory in tokens
        free_strings(tokens, nToken);

        // Clear input buffer
        memset(input, 0, MAX_INPUT);
    }

    // should not return here (since user will exit through a command)
    perror("Undefined behavior detected.");
    return 1;
}
