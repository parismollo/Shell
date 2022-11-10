// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>


// variables
static char *prompt_line = NULL;

char * slash_read_input() {
  // [TODO] Display path on prompt, see project doc. Above temporary solution:
  char * prompt_path = "> ";

  // If prompt line is not null, we free then set to null for next use:
  if (prompt_line) {
    free(prompt_line);
    prompt_line = NULL;
  }

  // Read line from prompt:
  prompt_line = readline(prompt_path);

  // If prompt_line not empty or null, save to history: 
  if(prompt_line && *prompt_line) {
    add_history(prompt_line);
  }
  return prompt_line;
}


int main() {
  while(true) {
    // Step 1: Read input:
    char * input = slash_read_input();
    printf("User input was: %s\n",input); // Remove comment from this line to test
    
    // Step 2: Interpret input:
    // Step 3: Execute input:
  }
}
