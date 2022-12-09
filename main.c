#include "slash.h"

int main() {
  signal(SIGINT, catchSignal);
  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    if(prompt_line != NULL) {
      // Step 2: Interpret input:
      char **tokens = slash_interpret(prompt_line);
      // Step 3: Execute input:
      if(tokens) {
        char*** paths = get_tokens_paths(tokens);
        char* temp_tokens[MAX_ARGS_NUMBER + 1];
        temp_tokens[0] = tokens[0];
        temp_tokens[1] = NULL;
        for(int i=2;i<MAX_ARGS_NUMBER;i++)
          temp_tokens[i] = NULL;
        
        
        for(int i=0; paths[i]!=NULL; i++) {
          for(int j=0; paths[i][j]!=NULL; j++) {
            printf("%s\n", paths[i][j]);
          }
        }

        //exec_all(paths, temp_tokens, 1);

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
