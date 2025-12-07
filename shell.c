#include <stdio.h>
#include "vect_token.h"
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include "vector.h"

#define MAX_INPUT 256
static char *previous = NULL; // holds on to previous command

void make_pipe(char** args);
void detect_tokens(char **args);
void nullify(char **args);
void exec_pipe(char** left, char** right);
void make_redirect(char** args);
void execute(char** args);
void redirect(char** args, char* filename, int dir);


// sets previous to the input 
void set_previous(char *input) {
  input[strlen(input)-1] = '\0'; // replace "\n" with end of string
  char *previous_pos = strstr(input, "prev"); //points to first instance of	prev
  if (previous_pos == NULL) { // if the last command is not "previous", then replace previous with new input
    free(previous);
    previous = strdup(input);
  }
}

// parses through the input string to split and execute sequences
void parse(char *input) {
    char **current;
    char *args[MAX_INPUT]; //current sequence arguments
    nullify(&args);
    int index = 0;

    // Use vector_t to store the input
    vector_t* token_vect = get_tokens(input); // tokenize input, array of strings
    char **tokens = token_vect->data;
    assert(tokens != NULL);

    current = tokens;
    while (*current != NULL) { //while there is still input
      if (strcmp(*current,";") == 0) { // once we hit a semicolon, execute on args before it
        index++;
        args[index] = NULL;
        detect_tokens(args); 
        nullify(&args);
        index = 0;
  
        ++current;
        continue;
      }

      args[index] = *current;
      ++current;
      index++;
    }

    detect_tokens(args); // execute on remaining args
    free_vector(token_vect);
}

// parse through given arg sequence, and determine whether to incorporate piping or redirecting
void detect_tokens(char **args) {
  if (contains_pipe(args)) {
    make_pipe(args);
  } else if (contains_redirect(args)) {
    make_redirect(args);
  } else {
    execute(args);
  }
} 

// clears the given argument string array
void nullify(char **args) {
  for (int i = 0; i < MAX_INPUT; i++) {
    args[i] = NULL;
  }
}

// check whether a given arg string array has a pipe
int contains_pipe(char** args) {
  for (int i = 0; args[i] != NULL; i++) {
    // printf("%s ", args[i]);
    if (strcmp(args[i], "|") == 0) {
      return true;
    }
  }
  return false;
}

// prepares a call to exec_pipe using the given arg string array
void make_pipe(char** args) {
  // iterate through args until | is hit
  for (int i = 0; args[i] != NULL; i++) { 
    if (strcmp(args[i], "|") == 0) { // on hit, split args into a left and right string array
      // all args left of the pipe
      char *left[MAX_INPUT];
      nullify(&left);
      for (int j = 0; j < i; j++) {
        left[j] = args[j];
      }
      // all args right of the pipe
      char *right[MAX_INPUT];
      nullify(&right);
      for(int k = 0; args[k+i+1] != NULL; k++) {
        right[k] = args[k+i+1];
      }
      exec_pipe(left, right);
      return;
    } 
  }
}

// execute a piping command
void exec_pipe(char** left, char** right) {
  // fork child A
  if (fork() == 0) {
    // create pipe
    int pipe_fds[2];
    assert(pipe(pipe_fds) == 0);
    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    // fork child B
    if (fork() == 0) {
      // close pipe input
      if (close(read_fd) == -1) {
        perror("Error closing pipe input");
        exit(1);
      }

      // close stdout to replace it with input end of pipe
      if (close(1) == -1) {
        perror("Error closing stdout");
        exit(1);
      }
      assert(dup(write_fd) == 1);

      // execute left side of pipe
      detect_tokens(left);
      _exit(0);
    } else {    
      // close pipe output
      if (close(write_fd) == -1) {
        perror("Error closing pipe output");
        exit(1);
      }

      // close stdin to replace it with output end of pipe
      if (close(0) == -1) {
        perror("Error closing stdin");
        exit(1);
      }
      assert(dup(read_fd) == 0);

      // execute right side of pipe (works recursively)
      detect_tokens(right);
      wait(NULL);
      exit(0);
    }
  } else {
    wait(NULL);
  }
} 

// check if the arg string array has a redirect
int contains_redirect(char** args) {
  for (int i = 0; args[i] != NULL; i++) {
    if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
      return true;
    }
  }
  return false;
}

// prepares a call to redirect using the given arg string array
void make_redirect(char** args) {
  char *redirect_args[MAX_INPUT];
  // iterate through all of args until either < or > is hit
  for (int i = 0; args[i] != NULL; i++) {
    if (strcmp(args[i], "<") == 0) { // if < is hit, then dir = 0
      for (int j = 0; j < i; j++) {
        redirect_args[j] = args[j];
      }
      redirect_args[i] = NULL;
      redirect(redirect_args, args[i+1], 0);
    } else if (strcmp(args[i], ">") == 0) { // if > is hit, then dir = 1
      for (int j = 0; j < i; j++) {
        redirect_args[j] = args[j];
      }
      redirect_args[i] = NULL;
      redirect(redirect_args, args[i+1], 1);
    }
  }
  
}


// performs a redirect on the given args and file in the given dir
void redirect(char** args, char* filename, int dir) {
  // fork to redirect input/output and then execute
  if (fork() == 0) {
    if (dir == 0) { // command takes file as input
      if (close(0) == -1) {
        perror("Error closing stdin");
        _exit(1);
      }
      int fd = open(filename, O_RDONLY); // open file for reading
      assert(fd == 0);
    } else {
      if (close(1) == -1) { // command gives output to file
        perror("Error closing stdout");
        _exit(1);
      }
      int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // either open file for writing, create it, or clear it
      assert(fd == 1);
    }
    // execute given command since it cannot be a built-in
    execvp(args[0], args);
    perror("Error - execlp failed");
    _exit(1);
  } else {
    wait(NULL);
  }
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

// executes a simple command (no piping, no redirecting)
void execute(char** args) {
  // if user just hits enter, ignore
  if (args[0] == NULL) {
    return;
  }

  if (strcmp(args[0], "exit") == 0 && arglen(args) == 1) { // exit built-in command
    printf("Bye bye.\n");
    free(previous); 
    exit(0);
  } else if (strcmp(args[0], "cd") == 0 && arglen(args) == 2) { // cd built-in command
    chdir(args[1]); 
  } else if (strcmp(args[0], "source") == 0 && arglen(args) == 2) { // source built-in command
    FILE *source = fopen(args[1], "r"); // open file given
    if (source == NULL) { // if file does not exist, print error message
      printf("Error trying to open %s\n", args[1]);
    }
    char line[MAX_INPUT];
    // read and parse each line
    while (fgets(line, MAX_INPUT, source)) {
      parse(line);
    }
  } else if (strcmp(args[0], "prev") == 0 && arglen(args) == 1) { // previous built-in command
    if (previous != NULL) { // only do something if there is a previousious command
      printf("%s\n", previous);
      parse(previous);
    } else {
      printf("No prev commmand to execute!\n");
    }
  } else if (strcmp(args[0], "help") == 0 && arglen(args) == 1) { // help built-in command
    printf("Help Menu:\n");
    printf("cd : switch the directory\n");
    printf("source [filename] : takes an existing file by filename and makes each line within it a command\n");
    printf("previous : executes the last command\n");
    printf("exit : exit the shell\n");
    printf("help : prints all commands that are native to the shell\n");
  } else { // if given args do not match a built-in command, fork and use execvp
    if (fork() == 0) {
      execvp(args[0], args);
      printf("%s : command not found \n", args[0]);
      _exit(1);
    } else {
      wait(NULL);
    }
  }
}

//main method for user input to mini shell
int main(int argc, char **argv) {
  //welcome message
  printf("Welcome to mini-shell.\n");
 
  // keep running the shell program indefinitely
  while(1) {
    printf("shell $ ");

    char input[MAX_INPUT];
    //read input 
    if (fgets(input, MAX_INPUT, stdin) == NULL) { //if input = NULL (CTRL-D)
      //end program
      printf("Bye bye.\n");
      free(previous);
      return 0;
    } 
    parse(input); 
    set_previous(input); // save the input line as the previous command
  }
  free(previous); // free any remaining allocated memory
  return 0;
}
