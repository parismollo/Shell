// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "slash.h"


// variables
static char *prompt_line = NULL;

// static command library[] = {
//   {"exit", slash_exit}
// };

int slash_exit(char **args) {
  // Expects something like:
    // args[0] = ["1"]
    // args[1] = ["Message error"]
  int status = atoi(args[0]);
  if (status > 0) {
    perror(args[1]);
  }
  exit(status);
}

void slash_read_input() {
  // [TODO] Display path on prompt, see project doc. Above temporary solution:
  char * prompt_path = "> ";

  // If prompt line is not null, we free then set to null for next use:
  if (prompt_line) {
    free(prompt_line);
    prompt_line = NULL;
  }

  // Read line from prompt and update prompt line variable:
  prompt_line = readline(prompt_path);

  // If prompt_line not empty or null, save to history: 
  if(prompt_line && *prompt_line) {
    add_history(prompt_line);
  }
}

char **slash_interpret_input(char *line) {
  int len = 0;
  // Initial capacity of 10 words
  int capacity = 10;

  // We need an array os strings, so a pointer for storing one string and a double pointer to store multiple.
  char **tokens = malloc(sizeof(char *) * capacity);

  // Define delimeters
  char delim[] = " ";

  // Get first token
  char *t = strtok(line, delim); 

  // Loop over other tokens
  while(t != NULL) {
    // Assign string to a pointer in tokens.
    tokens[len] = t;
    
    // Increase capacity if necessary
    capacity = capacity * 2;
    tokens = realloc(tokens, capacity * sizeof(char*));
    
    // Read next token
    t = strtok(NULL, delim);

    // Update len
    len++;
  }
  // We set to null the last element so we know when to stop while looping.
  tokens[len] = NULL; 
  return tokens;
}


int main() {
  while(true) {
    // Step 1: Read input and update prompt line variable:
    slash_read_input();
    
    // Step 2: Interpret input:
    char **tokens = slash_interpret_input(prompt_line);
    
    // Step 3: Execute input:

    // Step 4: Clean memory:
    free(tokens);
  }
}
