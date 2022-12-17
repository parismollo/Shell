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
// #include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


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
char** joker_expansion(char* path);

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
void free_double_ptr(char** ptr);
void catchSignal(int signal);
int joker_cmp(char* joker, char* name);

char** get_paths(char** input, char** output);
char*** get_tokens_paths(char** tokens);
void exec(char** tokens);
void exec_all(char*** paths, char** tokens, int index);
void free_triple_ptr(char*** ptr);
void disp_double_ptr(char** ptr);
void disp_triple_ptr(char*** ptr);
char** flat_triple_tab(char*** tab);
char* remove_slashes(char* str);
char* copy_str(char* str);

#endif