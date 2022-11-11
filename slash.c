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
#include "slash.h"


// variables
static char *prompt_line = NULL;

static int exec_loop   = 1, // Variable pour sortir du while dans le main
           exit_status = 0; // Par défaut, une valeur de sortie de 0

static command library[] = {
  {"exit", slash_exit},
  {"pwd",  slash_pwd}
};

int slash_exit(char **args) {
  // Could expect something like this (for internal use):
  //   args[1] = ["1"]
  //   args[2] = ["Message error"]

  // Permet de sortie de la boucle while dans le main
  exec_loop = 0;
  
  // external use (from terminal)
  if(args[1] == NULL) {return 0;}

  // internal use (from other fonctions)
  int status = atoi(args[1]);
  if (status > 0 && args[2] != NULL) {
    perror(args[2]);
  }
  
  // Valeur pour faire l'exit dans le main  
  exit_status = status;

  //exit(status);
  return status;
}

void slash_read() {
  // [TODO] Display path on prompt, see project doc. Above temporary solution:
  char * prompt_path = "> ";

  // If prompt line is not null, we free then set to null for next use:
  if (prompt_line) {
    free(prompt_line);
    prompt_line = NULL;
  }

  // Read line from prompt and update prompt line variable:
  prompt_line = readline(prompt_path);

  // If prompt_line not empty or null, save to history: 
  if(prompt_line && *prompt_line) {
    add_history(prompt_line);
  }
}

char **slash_interpret(char *line) {
  int len = 0;

  // Initial capacity of 10 words
  int capacity = 10;

  // We need an array os strings, so a pointer for storing one string and a double pointer to store multiple.
  char **tokens = malloc(sizeof(char *) * capacity);

  // Define delimeters
  char delim[] = " ";

  // Get first token
  char *t = strtok(line, delim); 

  // Loop over other tokens
  while(t != NULL) {
    // Assign string to a pointer in tokens.
    tokens[len] = t;
    
    // Increase capacity if necessary 
    capacity = capacity * 2;
    tokens = realloc(tokens, capacity * sizeof(char*));
    
    // Read next token
    t = strtok(NULL, delim);

    // Update len
    len++;
  }
  // We set to null the last element so we know when to stop while looping.
  tokens[len] = NULL; 
  return tokens;
}


void slash_exec(char **tokens) {
  // tokens should look like [["cd"], ["path/to/somewhere"]]
  
  // If there is nothing to execute, leave this function. 
  if(tokens[0] == NULL) return;
  
  // Loop over all builtin methods.
  int library_size = sizeof(library) / sizeof(library[0]);
  for (int i = 0; i < library_size; i++) {

    // If we find a match, execute with arguments
    if(!(strcmp(tokens[0], library[i].name))) {
      library[i].function(tokens);
      if(!exec_loop) // On sort directement si on vient d'executer "exit" (ou une autre fonction qui doit stopper le programme)
        return;
    }
  }
}

int main() {

  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    // Step 2: Interpret input:
    char **tokens = slash_interpret(prompt_line);
    
    // Step 3: Execute input:
    slash_exec(tokens);

    // Step 4: Clean memory:

    ////////////////////////////////////////////////
    // ATTENTION PROBLEME DE MEMOIRE ICI A REGLER //
    ////////////////////////////////////////////////

    //for(int i=0;i<10;i++)
      //free(tokens[i]);
    
    free(tokens);
  }
  
  rl_clear_history();
  return exit_status;
}

// PWD auxiliary functions

// Vérifier si un repertoire correspond à la racine
int is_root(DIR *dir) {
    struct stat st;
    int fd;

    fd = dirfd(dir);
    if(fd < 0) {
        perror("descripteur de fichier");
        return -1;
    }
    if(fstat(fd, &st) == -1) {
        perror("erreur fstat");
        return -1;
    }
    ino_t ino = st.st_ino;
    dev_t dev = st.st_dev;

    if(lstat("/", &st) == -1) {
        perror("erreur lstat dans is_root");
        return -1;
    }

    return st.st_ino == ino && st.st_dev == dev;
}

// Récuperer le nom d'un dossier grâce à son parent
char* get_dirname(DIR* dir, DIR* parent) {
  struct dirent *entry;
  struct stat st;

  if(fstat(dirfd(dir), &st) < 0) {
    perror("erreur fstat dans get_dirname");
    return NULL;
  }

  ino_t target_ino = st.st_ino;
  dev_t target_dev = st.st_dev;
  rewinddir(parent);

  // On parcourt les entrées du parent pour trouver notre repertoire
  while((entry=readdir(parent))) {
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
    if(fstatat(dirfd(parent), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0) {
	    perror("erreur stat dans get_dirname");
	    return NULL;
    }
    // On compare les ino et dev
    if(st.st_ino == target_ino && st.st_dev == target_dev)
      return entry->d_name;
  }

  return NULL;
}

// Permet de mettre une chaîne de caractère avant une autre
// Cette fonction s'utilise exclusivement pour pwd car elle ajoute
// également un '/' dans le chaine "buffer"
int push_string(char* buffer, char* str) {
  size_t s1 = strlen(str), buf_size = strlen(buffer);
  if(buf_size + s1 + 1 >= PATH_MAX) { // On vérifie qu'on depasse pas la taille max
    perror("Pas assez de place dans le buffer");
    return 1;
  }
  memmove(buffer + s1 + 1, buffer, buf_size); // On decalle ce qu'il y a dans buffer vers la droite
  buffer[0] = '/'; // On met un / au debut
  memmove(buffer + 1, str, s1); // On ecrit après le / le nom du dossier(str) dans le buffer
  return 0;
}

// PWD affiche la (plus précisément, une) référence absolue du répertoire de travail courant
int slash_pwd(char** args) {
  // Normalement -L par defaut. Pour l'instant -L ne fonctionne pas
  // Donc si pas d'argument on quitte. Pareil si l'arg est différent de "-P"
  if(args[1] == NULL || strcmp(args[1], "-P") != 0) {
    // Afficher une aide ? help() ?
    return 1;
  }

  DIR* dir = NULL, *parent = NULL;
  int parent_fd = -1;

  // buffer qui va contenir notre path
  // Il ne doit pas dépasser PATH_MAX
  char* buffer = malloc(PATH_MAX);
  if(buffer == NULL) {
   perror("malloc dans slash_cwd");
   goto error;
  }
  // On remplie notre buffer avec des '\0'
  memset(buffer, 0, PATH_MAX);

  dir = opendir(".");
  if(dir == NULL)
    goto error;

  // On vérifie si dir est la racine. Si oui, on peut quitter.
  if(is_root(dir)) {
    buffer[0] = '/';
    closedir(dir);
    printf("%s\n", buffer);
    free(buffer);
    return 0;
  }

  char* name = NULL;
  do {
    parent_fd = openat(dirfd(dir), "..", O_RDONLY | O_DIRECTORY);
    if(parent_fd < 0)
      goto error;
    parent = fdopendir(parent_fd);
    if(!parent)
      goto error;
    parent_fd = -1;

    name = get_dirname(dir, parent);
    if(name == NULL)
      goto error;
    if(push_string(buffer, name) != 0)
      goto error;
    closedir(dir);
    dir = parent;
  } while(!is_root(dir));

  closedir(dir);

  printf("%s\n", buffer);
  free(buffer);
  return 0;

  error:
    if(parent_fd < 0)
      close(parent_fd);
    if(buffer)
      free(buffer);
    if(dir)
      closedir(dir);
    if(parent)
      closedir(parent);
    perror("erreur dans slash_pwd");
    return 1;
}

