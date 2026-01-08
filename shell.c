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
#define MAX_CWD 1024
#define HALF_INPUT (MAX_INPUT / 2)
#define QUARTER_INPUT (MAX_INPUT / 4)

// Redirection options
#define INPUT_REDIR 1
#define OUTPUT_REDIR 0

// Child Error Status
#define ERROR_SUCCESS 0x1
#define ERROR_EXIT 0xF
#define ERROR_FAIL 0x10

// max number of command sequence
#define MAX_SEQ 8

// symbols for scanner
const char haystack[] = "()|<>";

// Global int flag for exiting
static int exit_flag = 0;

// Function declarations
char** parse(char* input, int* size);
void execute(int argc, char** argv);
static void _execute_cmd(int argc, char** argv);
static void _execute_part(int start, int end, char** argv);
static void _handle_pipe(int pivot, int argc, char** argv);
static void _handle_redirection(int type, int pivot, int argc, char** argv);
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
    char** tokens = (char **) malloc(sizeof(char*) * (num_tok + 1));

    // copy tokens into the new buffer
    for(int i = 0; i < num_tok; i++) {
        // local variables
        size_t length = strlen(tok_vect->data[i]) + 1; // include null-terminated char

        // allocate memory for each string
        tokens[i] = (char *) malloc(length * sizeof(char));

        // copy string into result array
        strncpy(tokens[i], tok_vect->data[i], length);    
    }

    // Set null terminator
    tokens[num_tok] = NULL;

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
            
            // get last redirection operator
            else if (operator == '<' || operator == '>') {
                redir_pos = i;
            }
        }
    }

    // Prioritize operators
    return (pipe_pos > 0) ? pipe_pos : ((redir_pos > 0) ? redir_pos : -1);
}

// helper function to execute fundamental command
static void _execute_cmd(int argc, char** argv) {
    char* command = argv[0];
    DEBUG_PRINT("_execute_cmd: command = \"%s\"\n", command);

    // Built-in commands
    if(strncmp(command, "exit", strlen("exit")) == 0) {         // EXIT FUNCTION
        // Free the memory from argv
        free_strings(argv, argc);

        // Exit the entire program
        printf("Bye bye.\n");
        exit(0);
    } else if (strncmp(command, "cd", strlen("cd")) == 0) {     // CD FUNCTION
        // no path given (use default path)
        if (argc < 2 && chdir(getenv("HOME")) != 0) {
            fprintf(stderr, "cd: %s\n", strerror(errno));
            return;
        } else if(chdir(argv[1]) != 0) {  // use path given
            fprintf(stderr, "cd: %s\n", strerror(errno));
            return;
        }
        return;
    } else if (strncmp(command, "help", strlen("help")) == 0) {
        printf("----- Help Menu -----\n");
        printf("cd : switch the directory\n");
        printf("source [filename] : takes an existing file by filename and executes each line\n");
        printf("previous : executes the last command\n");
        printf("exit : exit the shell\n");
        printf("help : prints all commands that are native to the shell\n");
        return;
    }
    // Fork a child and execute the command
    pid_t child = fork();
    
    if (child == 0) {           // CHILD
        // use execute helper function
        execvp(argv[0], argv); // replaces the process

        // Command not found error
        fprintf(stderr, "%s: command not found\n", command);
        _exit(EXIT_FAILURE);
    } else {                    // PARENT
        waitpid(child, NULL, 0);
        return;
    }
}

/**
 * Executes the section of the arguments; end exclusive
 */
static void _execute_part(int start, int end, char** argv) {
    // get the number of arguments
    int argc = end - start;

    // allocate memory for the list of tokens
    char** part = (char **) malloc(sizeof(char*) * (argc + 1));

    for(int i = 0, pos = start; i < argc && pos < end; i++, pos++) {
        int tok_len = strlen(argv[pos]) + 1;

        // allocate memory for the string token
        part[i] = (char *) malloc(sizeof(char) * tok_len);

        // copy the string into the new string
        strncpy(part[i], argv[pos], tok_len);
    }  

    // Null terminator
    part[argc] = NULL;

    // execute the command
    execute(argc, part);

    // free the memory here
    free_strings(part, argc + 1);
}

// Handles the logic for piping
static void _handle_pipe(int pivot, int argc, char** argv) {
    // create a pipe
    int pipe_fd[2];
    assert(pipe(pipe_fd) != -1);

    // set read and write fd
    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    // execute the pipes
    pid_t childA = fork();

    // fork child a
    if (childA == 0) {
        // fork child B
        pid_t childB = fork();
        if (childB == 0) {
            // close read side and stdout
            close(STDOUT_FILENO);
            close(read_fd);

            // replace stdout with write fd of pipe
            assert(dup2(write_fd, STDOUT_FILENO) != -1);
            close(write_fd);

            // execute command 1
            _execute_part(0, pivot, argv);

            // exit out of child B
            _exit(0);
        }
        // close write side and stdin
        close(STDIN_FILENO);
        close(write_fd);

        // replace stdin with read_fd of pipe
        assert(dup2(read_fd, STDIN_FILENO) != -1);
        close(read_fd);

        // execute command 2
        _execute_part(pivot + 1, argc, argv);

        // wait for child B
        waitpid(childB, NULL, 0);

        // exit out of child A
        _exit(0);
    } else {
        // close unused pipe in parent
        close(read_fd);
        close(write_fd);

        // wait for child A to finish
        waitpid(childA, NULL, 0);
    }
}

// Handles the logic for redirection
static void _handle_redirection(int type, int pivot, int argc, char** argv) {
    assert(type == 0 || type == 1);
    assert(argv != NULL);
    assert(pivot != 0);
    assert(argc > 2);

    pid_t child = fork();
    // Input Redirection (1)
    if (child == 0) {
        if (pivot + 1 >= argc) {
            fprintf(stderr, "No file indicated for redirection.\n");
            _exit(EXIT_FAILURE);
        }

        // get file path 
        char* file_path = argv[pivot + 1];

        // Input Redirection (1)
        if (type) {
            // close stdin for input
            close(STDIN_FILENO);

            // open the file for reading
            int fd = open(file_path, O_RDONLY);
            if(fd != STDIN_FILENO) {
                fprintf(stderr, "Failed to open file on file descriptor (%d). Returned: %d\n", STDIN_FILENO, fd);
                _exit(EXIT_FAILURE);
            }
        }
        // Output Redirection (0)
        else {
            // closes stdout for output
            close(STDOUT_FILENO);

            // open the file for writing
            int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd != STDOUT_FILENO) {
                fprintf(stderr, "Failed to open file on file descriptor (%d). Returned: %d\n", STDIN_FILENO, fd);
                _exit(EXIT_FAILURE);
            }
        }
        // execute the function up till the pivot
        _execute_part(0, pivot, argv);
        _exit(EXIT_SUCCESS);
    } else {
        waitpid(child, NULL, 0);
    }
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
            _execute_part(1, argc - 1, argv);
            return;
        } 

        // execute the command
        _execute_cmd(argc, argv);
    } else {
		// Check operator
		char operator = argv[pivot][0];
		
		switch (operator) {
			case ';':
                // execute each part
                _execute_part(0, pivot, argv);
                _execute_part(pivot + 1, argc, argv);
				break;
            case '|':
                _handle_pipe(pivot, argc, argv);
                break;
            case '>':
                _handle_redirection(OUTPUT_REDIR, pivot, argc, argv);
                break;
            case '<':
                _handle_redirection(INPUT_REDIR, pivot, argc, argv);
                break;
			default:
                fprintf(stderr, "Unknown operator %c\n", operator);
                _exit(EXIT_FAILURE);
				break;
		}
		return;
    }
}

// free the strings in the allocated array
static void free_strings(char** arr, int n) {
	for(int i = 0; i < n; i++) {
		free(arr[i]);
	}
	free(arr);
}

// main method for user input to mini shell
int main(int argc, char **argv) {
    // welcome message
    printf("Welcome to mini-shell.\n");

    // keep running the shell program indefinitely
    while(1) {
        // print shell heading
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
