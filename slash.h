#ifndef SLASH
#define SLASH

#define MAX_ARGS_NUMBER 4096
#define MAX_ARGS_STRLEN 4096

// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h> 
#include <errno.h>
#include <signal.h>


extern char *colors[];
extern int PRINT_PWD; // Variable auxiliaire pour cd
extern char PATH[PATH_MAX];
extern char* prompt_line;
extern int exec_loop;
extern int exit_status;

// Structures
typedef struct command {
  char *name;
  int (*function) (char **args);

}command;

// Slash Fonctions
void slash_read();
char **slash_interpret(char *line);
void slash_exec(char **tokens);
int slash_exit(char **args);
int slash_help(char **args);
int slash_pwd(char** args);
char *slash_get_prompt();
int slash_cd(char **args);


// Auxiliary
int main();

int is_root(DIR *dir);
char* get_dirname(DIR* dir, DIR* parent);
int push_string(char* buffer, char* str);
char * get_color(int n);
char * get_exit_status();
char * get_shorter_path(char * string, int max_path_size);
void catchSignal(int signal);
char* real_path(char* p);
void error_chdir();
int slash_cd_aux(char option, const char* pwd, char *args);

#endif