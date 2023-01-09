#ifndef SLASH
#define SLASH

#define MAX_ARGS_NUMBER 4096
#define MAX_ARGS_STRLEN 4096

// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h> 
#include <errno.h>
#include <signal.h>
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
  char* name;
  int (*function) (char** args);

}command;

typedef struct {
  size_t capacity;
  size_t length;
  char* data;
}string;

// Fonctions

/* slash.c */
int slash_help(char** args);
int slash_exit(char** args);
void slash_read();
char** slash_interpret(char* line);
void slash_exec(char** tokens);
int slash_pwd(char** args);
char* slash_get_prompt();
int slash_cd(char** args);
char*** get_tokens_paths(char** tokens);


/* slash_aux.c */
void catchSignal(int signal);
char* get_shorter_path(char* string, int max_path_size);
char* get_exit_status();
char* get_color(int n);
int is_root(DIR *dir);
char* get_dirname(DIR* dir, DIR* parent);
int push_string(char* buffer, char* str);
char* real_path(char* p);
void error_chdir();
int slash_cd_aux(char option, const char* pwd, char *args);
void exec(char** tokens);
char* copy_str(char* str);
int file_exists(char* file, int is_directory);
char* remove_slashes(char* str);
char** concat(char** target, int* target_size, char** source);
char** flat_triple_tab(char*** tab);
void free_double_ptr(char** ptr);
void free_triple_ptr(char*** ptr);
void disp_double_ptr(char** ptr);
void disp_triple_ptr(char*** ptr);


/* jokers.c */
char** joker_expansion(char* path, int only_dir);
int joker_cmp(char* joker, char* name);
char** get_paths(char** input, char** output, int only_dir);
char** cut_path(char* path, char* delim);
char** total_expansion(char* path, int only_dir);
char*** total_expansion_aux(string* path, char* pattern, int only_dir, char*** exp, int* size, int* cap);
int prefix(char* str, char* pre);

/* mystring.c */

string* string_new(size_t capacity);
void string_delete(string* str);
int string_append (string* dest, char* src);
void string_truncate (string* str, size_t nchars);

/* slash_redirections.c */
void redirection(char** flat_tab);
int is_valid_operator(char* str);

/* slash_pipe.c */
void apply_pipes(char** tokens);

#endif