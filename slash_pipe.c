#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "mystring.c"

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
  // convert tab dans un seul string avec mystring(string new et string append) et ensuite on utilise strtok!!
  string* cmd = string_new(10);
  for(int i=0; tab[i]!=NULL; i++) {
    string_append(cmd, tab[i]);
    string_append(cmd, " ");
  }
  int len = 0;
  int capacity = 100;
  char** tokens = malloc(sizeof(char*) * capacity);
  char* token = strtok(cmd->data, "|");
  while(token != NULL) {
    if(len >= capacity - 1) {
      /*realloc*/
    }
    tokens[len] = token;
    len++;
    token = strtok(NULL, "|");
  }
  tokens[len] = NULL;
  return tokens;
}

char** get_args(char* str) {
  int capacity = 0;
  int len = 0;
  char** tokens = malloc(sizeof(char*) * capacity);
  char* token = strtok(str, " ");

  while(token != NULL) {
    if(len >= capacity - 1) {
      /*realloc*/
    }
    tokens[len] = token;
    len++;
    token = strtok(NULL, " ");
  }
  tokens[len] = NULL;
  return tokens;
}
void f(char** tab) {
  // int fd = open("log", O_WRONLY | O_APPEND, 0666);
  int fd_prev[2], fd_next[2];
  // [TODO]: Gestion erreur de pipe
  pipe(fd_prev); // cmd impair -> cmd pair
  pipe(fd_next); // cmd pair -> cmd impair

  for(int i=0; tab[i] != NULL; i++) {
    if(i == 0) {
      char** args = get_args(tab[i]);
      int pid = fork();
      if(pid == 0) {
        close(fd_prev[1]);
        close(fd_next[0]);
        
        dup2(fd_next[1], STDOUT_FILENO);
        
        close(fd_next[1]);
        
        execvp(args[0], args);
      }
    
    }else if (tab[i + 1] == NULL) {
      char** args = get_args(tab[i]);
      int pid = fork();
      if(pid == 0) {
        if((i+1)%2 == 0) {
          close(fd_next[0]); 
          close(fd_prev[1]);
          
          dup2(fd_prev[0], STDIN_FILENO);
          
          close(fd_prev[0]);
        }else {
          close(fd_prev[0]); 
          close(fd_next[1]);
          
          dup2(fd_next[0], STDIN_FILENO);
          
          close(fd_next[0]);
        }
        execvp(args[0], args);
      }
    }
    
    else {
      char** args = get_args(tab[i]);
      int pid = fork();
      if(pid == 0) {
        if(i % 2 == 0) {
          close(fd_next[0]); 
          close(fd_prev[1]);
          
          dup2(fd_prev[0], STDIN_FILENO);
          dup2(fd_next[1], STDOUT_FILENO);
          
          close(fd_prev[0]); 
          close(fd_next[1]);
        }else{
          close(fd_next[1]); 
          close(fd_prev[0]);
          
          dup2(fd_next[0], STDIN_FILENO);
          dup2(fd_prev[1], STDOUT_FILENO);
          
          close(fd_next[0]); 
          close(fd_prev[1]);
        }
        execvp(args[0], args);
      }
      // wait(NULL);
    }
  }
}

int main() {
  char ** flat_tab = malloc(7 * sizeof(char*));
  if(flat_tab == NULL) {perror("malloc failed"); exit(1);}

  flat_tab[0] = malloc(strlen("ls")+1);
  strcpy(flat_tab[0], "ls");
  flat_tab[1] = malloc(strlen("test")+1);
  strcpy(flat_tab[1], "test");
  flat_tab[2] = malloc(strlen("|")+1);
  strcpy(flat_tab[2], "|");
  flat_tab[3] = malloc(strlen("cat")+1);
  strcpy(flat_tab[3], "cat");
  flat_tab[4] = malloc(strlen("|")+1);
  strcpy(flat_tab[4], "|");
  flat_tab[5] = malloc(strlen("wc")+1);
  strcpy(flat_tab[5], "wc");
  flat_tab[6] = NULL;
  
  if(contains_pipe(flat_tab) == 0) return 0;
  char** new_tab = get_new_tab(flat_tab);
  f(new_tab);
  return EXIT_SUCCESS; 
}