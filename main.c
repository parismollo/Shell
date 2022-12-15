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
      if(tokens) {
        char*** paths = get_tokens_paths(tokens);
        // disp_triple_ptr(paths);

        // for(int i=0; paths[i] != NULL; i++) {
        //   printf("DEBUT Résultat %d:\n", i);
        //   for(int j=0; paths[i][j] != NULL; j++) {
        //     printf("Résultat étoile : %s\n", paths[i][j]);
        //   }
        // }

        // char* temp_tokens[MAX_ARGS_NUMBER + 1];
        // temp_tokens[0] = tokens[0];
        // temp_tokens[1] = NULL;

        // for(int i=2;i<MAX_ARGS_NUMBER;i++)
        //   temp_tokens[i] = NULL;
        
        // disp_double_ptr(flat_triple_tab(paths));
        
        // exec_all(paths, temp_tokens, 1);

        char** flat_tokens = flat_triple_tab(paths);
        if(flat_tokens) {
          //disp_double_ptr(flat_tokens);
          slash_exec(flat_tokens);
          free_double_ptr(flat_tokens);
        }

        free_triple_ptr(paths);
        
        //slash_exec(tokens);
      }
      free(tokens);
    }
    
    free(prompt_line);
  } 
  // Clear readline history:
  rl_clear_history();
  return exit_status;
}
