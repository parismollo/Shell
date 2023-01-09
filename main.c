#include "slash.h"

int main() {

  struct sigaction action={0};
  memset(&action, 0, sizeof(struct sigaction));
  //action.sa_handler = catchSignal;
  action.sa_handler = SIG_IGN;
  //sigaction(SIGINT, &action ,NULL);

  // signal(SIGINT, catchSignal);

  // struct sigaction act = {0};
  // act.sa_handler = SIG_IGN;
  sigaction(SIGINT, &action, NULL); 
  sigaction(SIGTERM, &action, NULL);
  //slash ignore les signaux SIGINT et SIGTERM

  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    if(prompt_line != NULL) {
      // Step 2: Interpret input:
      char **tokens = slash_interpret(prompt_line);
      // Step 3: Execute input:
      if(tokens) {
        char*** paths = get_tokens_paths(tokens);
        char** flat_tokens = flat_triple_tab(paths);
        
        if(flat_tokens) {
          apply_pipes(flat_tokens);
          //slash_exec(flat_tokens);
          free_double_ptr(flat_tokens);
        }

        free_triple_ptr(paths);
      }
      free(tokens);
    }
    
    free(prompt_line);
  }
  // Clear readline history:
  rl_clear_history();
  return exit_status;
}
