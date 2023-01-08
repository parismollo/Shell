#include "slash.h"

int contains_pipe(char** tab) {
  for(int i=0; tab[i] != NULL; i++) {
    if(strcmp("|", tab[i])==0) {
      // printf("contains_pipe: true\n");
      return 1; // True
    }
  }
  // printf("contains_pipe: false\n");
  return 0; // False
}

char** get_new_tab(char** tab) {
  // [ls, test, |, cat, |, wc]
  // convert tab dans un seul string avec mystring(string new et string append) et ensuite on utilise strtok!!
  // ["ls test", "cat", "wc"]
  string* cmd = string_new(10);
  for(int i=0; tab[i]!=NULL; i++) {
    string_append(cmd, tab[i]);
    string_append(cmd, " ");
  }
  int len = 0;
  int capacity = 100;
  char** tokens = malloc(sizeof(char*) * capacity);
  if(tokens==NULL) {perror("malloc failed"); string_delete(cmd); exit(1);}
  // TODO malloc
  char* token = strtok(cmd->data, "|");
  while(token != NULL) {
    if(len >= capacity - 1) {
      char** ptr = realloc(tokens, capacity * 2);
      if(ptr == NULL) {
        perror("malloc");
        free_double_ptr(tokens);
        string_delete(cmd);
        exit(1);
      }
      tokens = ptr;
      capacity *= 2;
    }
    tokens[len] = copy_str(token);
    if(tokens[len] == NULL) {
      perror("malloc");
      free_double_ptr(tokens);
      string_delete(cmd);
      exit(1);
    }
    len++;
    token = strtok(NULL, "|");
  }
  tokens[len] = NULL;
  string_delete(cmd);
  
  return tokens;
}

char** get_args(char* str) {
  int capacity = 10;
  int len = 0;
  char** tokens = malloc(sizeof(char*) * capacity);
  if(tokens == NULL) {perror("malloc failed"); exit(1);}
  char* token = strtok(str, " ");

  while(token != NULL) {
    if(len >= capacity - 1) {
      char** ptr = realloc(tokens, capacity * 2 * sizeof(char*));
      if(ptr == NULL) {
        perror("error in get_args realloc");
        free(tokens);
        return NULL;
      }
      tokens = ptr;
      capacity *= 2;
    }
    tokens[len] = copy_str(token);
    if(tokens[len] == NULL)
      return NULL;
    len++;
    token = strtok(NULL, " ");
  }
  tokens[len] = NULL;
  return tokens;
}

void f(char** tab) {
  int fd_prev[2], fd_next[2];
  // [TODO]: Gestion erreur de pipe
  pipe(fd_prev); // cmd impair -> cmd pair
  pipe(fd_next); // cmd pair -> cmd impair

  for(int i=0; tab[i] != NULL; i++) {
    char** args = get_args(tab[i]);
    // int saved_stdout = dup(STDOUT_FILENO);
    // int saved_stdin = dup(STDIN_FILENO);
    // int saved_stderr = dup(STDERR_FILENO);
    if(fork() == 0) {
      if(i == 0) {
        close(fd_prev[1]);
        close(fd_next[0]);
        close(fd_prev[0]);
        dup2(fd_next[1], STDOUT_FILENO);
        close(fd_next[1]);
        execvp(args[0], args);
        return;
      // wait(NULL);
      }
      else if (tab[i + 1] == NULL) {
        if(i%2 == 0) {
          close(fd_next[0]); 
          close(fd_prev[1]);
          close(fd_next[1]);
          dup2(fd_prev[0], STDIN_FILENO);
          close(fd_prev[0]);
        }
        else {
          close(fd_prev[0]); 
          close(fd_next[1]);
          close(fd_prev[1]);
          dup2(fd_next[0], STDIN_FILENO);
          close(fd_next[0]);
        }
        execvp(args[0], args);
        return;
        // wait(NULL);
      }
      else {
        if(i % 2 == 0) {
          close(fd_next[0]); 
          close(fd_prev[1]);
          
          dup2(fd_prev[0], STDIN_FILENO);
          dup2(fd_next[1], STDOUT_FILENO);
          
          close(fd_prev[0]); 
          close(fd_next[1]);
        }
        else {
          close(fd_next[1]); 
          close(fd_prev[0]);
          
          dup2(fd_next[0], STDIN_FILENO);
          dup2(fd_prev[1], STDOUT_FILENO);
          
          close(fd_next[0]); 
          close(fd_prev[1]);
        }
        execvp(args[0], args);
      }
    }

    wait(NULL);

    // dup2(saved_stdout, STDOUT_FILENO);
    // close(saved_stdout);
    // dup2(saved_stdin, STDIN_FILENO);
    // close(saved_stdin);
    // dup2(saved_stderr, STDERR_FILENO);
    // close(saved_stderr);

    free_double_ptr(args);
  }
}

void apply_pipes(char** tokens) {
  if(!contains_pipe(tokens)) {
    redirection(tokens);
  }
  else {
    // int saved_stdout = dup(STDOUT_FILENO);
    // int saved_stdin = dup(STDIN_FILENO);
    // int saved_stderr = dup(STDERR_FILENO);

    // On résout les pipelines
    // char** new_tab = get_new_tab(tokens);
    // f(new_tab);
    // free_double_ptr(new_tab);

    // dup2(saved_stdout, STDOUT_FILENO);
    // close(saved_stdout);
    // dup2(saved_stdin, STDIN_FILENO);
    // close(saved_stdin);
    // dup2(saved_stderr, STDERR_FILENO);
    // close(saved_stderr);
  }
}

// int main() {
//   char ** flat_tab = malloc(7 * sizeof(char*));
//   if(flat_tab == NULL) {perror("malloc failed"); exit(1);}

//   // char* flat_tab[] = {"ls", "test", "|", "cat", NULL};

//   flat_tab[0] = malloc(strlen("ls")+1);
//   strcpy(flat_tab[0], "ls");
//   flat_tab[1] = malloc(strlen("test")+1);
//   strcpy(flat_tab[1], "test");
//   flat_tab[2] = malloc(strlen("|")+1);
//   strcpy(flat_tab[2], "|");
//   flat_tab[3] = malloc(strlen("cat")+1);
//   strcpy(flat_tab[3], "cat");
//   flat_tab[4] = malloc(strlen("|")+1);
//   strcpy(flat_tab[4], "|");
//   flat_tab[5] = malloc(strlen("wc")+1);
//   strcpy(flat_tab[5], "wc");
//   flat_tab[6] = NULL;
//   // input <- ls test | cat | wc
//   if(contains_pipe(flat_tab) == 0) return 0;
//   char** new_tab = get_new_tab(flat_tab);
//   // disp_double_ptr(new_tab);
//   f(new_tab);
//   free_double_ptr(new_tab);
//   free_double_ptr(flat_tab);

//   return EXIT_SUCCESS; 
// }
