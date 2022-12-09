#include "slash.h"

int main() {

  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = catchSignal;
  sigaction(SIGINT, &action ,NULL);

  // signal(SIGINT, catchSignal);

  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    if(prompt_line != NULL) {
      // Step 2: Interpret input:
      char **tokens = slash_interpret(prompt_line);
      // Step 3: Execute input:
      if(tokens) {slash_exec(tokens);}
      free(tokens);
    }
    
    free(prompt_line);
  } 
  // Clear readline history:
  rl_clear_history();
  return exit_status;
}
