#ifndef SLASH
#define SLASH

// Structures
typedef struct command {
  char *name;
  int (*function) (char **args);

}command;

// Fonctions
void slash_read();
char **slash_interpret(char *line);
void slash_exec(char **tokens);
int slash_exit(char **args);

#endif