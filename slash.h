#ifndef SLASH
#define SLASH

// Structures
typedef struct command {
  char *name;
  int (*function) (char **args);

}command;

// Fonctions
void slash_read_input();
char **slash_interpret_input(char *line);
int slash_exit(char **args);

#endif