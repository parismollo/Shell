// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "slash.h"


// variables
static char *prompt_line = NULL;

static int exec_loop = 1, exit_status = 0;

static command library[] = {
  {"exit", slash_exit}
};

int slash_exit(char **args) {
  // Could expect something like this (for internal use):
  //   args[1] = ["1"]
  //   args[2] = ["Message error"]

  // Permet de sortie de la boucle while dans le main
  exec_loop = 0;
  
  // external use (from terminal)
  if(args[1] == NULL) {return 0;}

  // internal use (from other fonctions)
  int status = atoi(args[1]);
  if (status > 0 && args[2] != NULL) {
    perror(args[2]);
  }
  
  // Valeur pour faire l'exit dans le main  
  exit_status = status;

  //exit(status);
  return status;
}

void slash_read() {
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

char **slash_interpret(char *line) {
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


void slash_exec(char **tokens) {
  // tokens should look like [["cd"], ["path/to/somewhere"]]
  
  // If there is nothing to execute, leave this function. 
  if(tokens[0] == NULL) return;
  
  // Loop over all builtin methods.
  int library_size = sizeof(library) / sizeof(library[0]);
  for (int i = 0; i < library_size; i++) {

    // If we find a match, execute with arguments
    if(!(strcmp(tokens[0], library[i].name))) {
      library[i].function(tokens);
      if(!exec_loop) // On sort directement si on vient d'executer "exit" (ou une autre fonction qui doit stopper le programme)
        return;
    }
  }
}

int main() {
  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    // Step 2: Interpret input:
    char **tokens = slash_interpret(prompt_line);
    
    // Step 3: Execute input:
    slash_exec(tokens);

    // Step 4: Clean memory:
    for(int i=0;i<10;i++)
      free(tokens[i]);
    free(tokens);
  }
  
  rl_clear_history();
  return exit_status;
}
