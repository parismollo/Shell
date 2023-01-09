#include "slash.h"


int check_pipe_redirection(char** tokens) {

  for(int i=0;tokens[i] != NULL;i++) {
    // Si c'est une redirection
    if(is_valid_operator(tokens[i])) {
      if(i == 0)
        return 0;
      if(is_valid_operator(tokens[i-1]))
        return 0;
      if(tokens[i+1] == NULL || is_valid_operator(tokens[i+1]))
        return 0;
    }
  }

  return 1;
}

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
  if(tokens==NULL) {perror("malloc failed"); string_delete(cmd); return NULL;}
  tokens[0] = NULL;

  char* token = cmd->data;
  char* tok_pipe;
  int another_one = 0;
  while((tok_pipe = strchr(token, '|')) && (another_one-- <= 0 || (tok_pipe = strchr(tok_pipe, '|')))) {
    *tok_pipe = '\0';
    
    // On vérifie le cas spécial où il y a un '>' avant le '|'
    size_t size = strlen(token);
    if(size > 0 && token[size-1] == '>') {
      *tok_pipe = '|';
      continue;
    }

    if(len >= capacity - 2) { // On laisse la place pour un NULL et un possible mot final
      char** ptr = realloc(tokens, capacity * 2);
      if(ptr == NULL) {
        perror("malloc");
        free_double_ptr(tokens);
        string_delete(cmd);
        return NULL;
      }
      tokens = ptr;
      capacity *= 2;
    }
    tokens[len] = copy_str(token);
    if(tokens[len] == NULL) { // perror in copy_str
      free_double_ptr(tokens);
      string_delete(cmd);
      return NULL;
    }
    tokens[++len] = NULL;

    *tok_pipe = '|';
    token = tok_pipe+1;
  }

  // On copie la derniere chaine
  if(*token != '\0') {
    tokens[len] = copy_str(token);
    if(tokens[len] == NULL) { // perror in copy_str
      free_double_ptr(tokens);
      string_delete(cmd);
      return NULL;
    }
    tokens[++len] = NULL;
  }

  string_delete(cmd);

  // Voici une méthode pour découpé les '|' à l'aide de strtok
  // Cependant, cette méthode ne fonctionne pas si il y a des '>|' ou '2>|' dans la string !
  // C'est pourquoi nous avons fait une autre méthode au dessus en utilisant 'strchr'
  // Mais cette méthode semble trop lente, car le test 4 des pipelines ne passe pas et renvoie:
  // "Le processus slash a dépassé le temps imparti et a été terminé par SIGALRM"

  // char* token = strtok(cmd->data, "|");
  // while(token != NULL) {
  //   if(len >= capacity - 1) {
  //     char** ptr = realloc(tokens, capacity * 2);
  //     if(ptr == NULL) {
  //       perror("malloc");
  //       free_double_ptr(tokens);
  //       string_delete(cmd);
  //       return NULL;
  //     }
  //     tokens = ptr;
  //     capacity *= 2;
  //   }
  //   tokens[len] = copy_str(token);
  //   if(tokens[len] == NULL) { // perror in copy_str
  //     free_double_ptr(tokens);
  //     string_delete(cmd);
  //     return NULL;
  //   }
  //   len++;
  //   tokens[len] = NULL;
  //   token = strtok(NULL, "|");
  // }
  // tokens[len] = NULL;
  // string_delete(cmd);
  
  return tokens;
}

char** get_args(char* str) {
  int capacity = 10;
  int len = 0;
  char** tokens = malloc(sizeof(char*) * capacity);
  if(tokens == NULL) {perror("malloc failed"); return NULL;}
  tokens[0] = NULL;
  char* token = strtok(str, " ");

  while(token != NULL) {
    if(len >= capacity - 1) {
      char** ptr = realloc(tokens, capacity * 2 * sizeof(char*));
      if(ptr == NULL) {
        perror("error in get_args realloc");
        free_double_ptr(tokens);
        return NULL;
      }
      tokens = ptr;
      capacity *= 2;
    }
    tokens[len] = copy_str(token);
    if(tokens[len] == NULL)
      return NULL;
    len++;
    tokens[len] = NULL;
    token = strtok(NULL, " ");
  }
  tokens[len] = NULL;
  return tokens;
}

int pipelines(char** tab) {
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stdin = dup(STDIN_FILENO);
  int saved_stderr = dup(STDERR_FILENO);
  int final_exit = -1;

  for(int i=0;tab[i] != NULL;i++) {
    char** args = get_args(tab[i]);
    int fd[2];
    if(pipe(fd) < 0)
      exit(1);
    pid_t pid = fork();
    if(pid == 0) {
      close(fd[0]);
      if(tab[i+1] != NULL)
        dup2(fd[1], 1);
      close(fd[1]);
      redirection(args);
      exit(exit_status);
    }
    else {
      close(fd[1]);
      dup2(fd[0], 0);
      close(fd[0]);
      waitpid(pid, &final_exit, WUNTRACED); // wait(NULL);
    }
    
    if(WIFSIGNALED(final_exit)) {
      exit_status = 255;
      break;
    }
    else if(WIFEXITED(final_exit)) {
      exit_status = WEXITSTATUS(final_exit);
    }

    free_double_ptr(args);
  }
  
  dup2(saved_stdout, 1);
  close(saved_stdout);
  dup2(saved_stdin, 0);
  close(saved_stdin);
  dup2(saved_stderr, 2);
  close(saved_stderr);

  return 0;
}

void apply_pipes(char** tokens) {
  // On vérifie si on a une erreur de syntaxe
  if(!check_pipe_redirection(tokens)) {
    return;
  }
  // On vérifie si il contient des pipes
  if(!contains_pipe(tokens)) {
    redirection(tokens);
  }
  else {
    // On résout les pipelines
    char** new_tab = get_new_tab(tokens);
    //disp_double_ptr(new_tab);
    pipelines(new_tab);
    free_double_ptr(new_tab);
  }
}