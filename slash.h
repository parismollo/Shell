#ifndef SLASH
#define SLASH

#define MAX_ARGS_NUMBER 4096
#define MAX_ARGS_STRLEN 4096

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


// Auxiliary
int is_root(DIR *dir);
char* get_dirname(DIR* dir, DIR* parent);
int push_string(char* buffer, char* str);

#endif