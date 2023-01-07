#include "slash.h"

int PRINT_PWD = 1; // Variable auxiliaire pour cd
char PATH[PATH_MAX];

// Colors
char *colors[] = {
  "\033[32m", // green
  "\033[91m", // red
  "\033[34m", // blue
  "\033[36m", // cyan
  "\033[00m",// white
  "\033[00m" // reset
};

// variables
char *prompt_line = NULL;
int exec_loop   = 1, // Variable pour sortir du while dans le main
           exit_status = 0; // Par défaut, une valeur de sortie de 0
static command library[] = {
  {"exit", slash_exit},
  {"pwd",  slash_pwd},
  {"help", slash_help},
  {"cd", slash_cd}
};

int slash_help(char **args) {
  char *help_text =
    "\n\033[34mSLASH LIB\033[00m\n"
    "\033[32m---------------\033[00m\n"
    "pwd [-L | -P]\n"
    "exit [val]\n"
    "cd [-L | -P] [ref | -]"
    "\n\033[32m---------------\033[00m\n";
  printf("%s", help_text);
  return 0;
}

int slash_exit(char **args) {
  // Could expect something like this (for internal use):
  //   args[1] = ["1"]
  //   args[2] = ["Message error"]

  // Permet de sortie de la boucle while dans le main
  exec_loop = 0;
  // If not args, return previous status
  if(args[1] == NULL) {return exit_status;}
  if(args[2] != NULL) {
    printf("slash: too many arguments, try help.\n");
    exec_loop = 1; // Don't quit, did not use function correctly.
    return 1;
  }

  // Otherwise, get status and update global variable
  int status = atoi(args[1]);

  // atoi returns 0, if can't convert.
  if(status == 0) {
    printf("slash: exiting with status 0\n");
    return 0;
  }

  // Maybe this is not useful: 
  // if (status > 0 && args[2] != NULL) {
  //   perror(args[2]);
  // }
  
  // Valeur pour faire l'exit dans le main  
  exit_status = status;

  //exit(status);
  return exit_status;
}

void slash_read() {
  // [TODO] Display path on prompt, see project doc. Above temporary solution:
  // char * prompt_path = "> ";
  char * prompt_path = slash_get_prompt();
  rl_outstream = stderr;
  // Read line from prompt and update prompt line variable:
  prompt_line = readline(prompt_path);
  // Detects EOF
  if(prompt_line == NULL) {
    // exit_status = 0;
    exec_loop = 0;
    free(prompt_path);
    return;
  }

  // If prompt_line not empty or null, save to history: 
  if(prompt_line && *prompt_line) {
    add_history(prompt_line);
  }
  free(prompt_path);
}

char **slash_interpret(char *line) {
  int len = 0;

  // We need an array of strings, so a pointer for storing one string and a double pointer to store multiple.
  char **tokens = malloc(sizeof(char *) * MAX_ARGS_NUMBER + 1);
  if(tokens == NULL) {exec_loop = 0; return NULL;}

  // Define delimeters
  char delim[] = " ";

  // Get first token
  char *t = strtok(line, delim); 

  // Loop over other tokens
  while(t != NULL) {
    // Assign string to a pointer in tokens.
    if(strlen(t) > MAX_ARGS_STRLEN) {
      printf("slash: Argument too long\n");
      return NULL;
    }
    tokens[len] = t;
    // Update len
    len++;
  
    if(len < MAX_ARGS_NUMBER) {
      // Read next token
      t = strtok(NULL, delim);
    }else {
      printf("slash: too many arguments");
      return NULL;
    }
  
  }
  // We set to null the last element so we know when to stop while looping.
  tokens[len] = NULL;
  // Temporary
  for(int i = len; i<100; i++)
    tokens[i] = NULL;

  return tokens;
}


void slash_exec(char **tokens) {
  // tokens should look like [["cd"], ["path/to/somewhere"]]
  
  // If there is nothing to execute, leave this function. 
  if(tokens[0] == NULL) return;

  // Loop over all builtin methods.
  int library_size = sizeof(library) / sizeof(library[0]);
  int no_command = 1;
  for (int i = 0; i < library_size; i++) {
  
    // If we find a match, execute with arguments
    if(strcmp(tokens[0], library[i].name) == 0) {
      no_command = 0;
      exit_status = library[i].function(tokens);
      if(!exec_loop) // On sort directement si on vient d'executer "exit" (ou une autre fonction qui doit stopper le programme)
        return;
    }
  }
  if(no_command) { // Si la commande n'est pas une commande interne
    exec(tokens);
  }
}

// PWD affiche la (plus précisément, une) référence absolue du répertoire de travail courant
int slash_pwd(char** args) {
  // Normalement -L par defaut. Pour l'instant -L ne fonctionne pas
  // Donc si pas d'argument on quitte. Pareil si l'arg est différent de "-P"
  if(args[1] == NULL || strcmp(args[1], "-L") == 0) {
    const char* pwd = getenv("PWD");
    if(pwd == NULL) {
      // perror("La variable d'environnement PWD n'existe pas");
      return 1;
    }
    printf("%s\n", pwd);
    return 0;
  }
  else if(strcmp(args[1], "-P") != 0) {
    // perror("Erreur argument. Essayer help\n");
    return 1;
  }

  DIR* dir = NULL, *parent = NULL;
  int parent_fd = -1;

  // buffer qui va contenir notre path
  // Il ne doit pas dépasser PATH_MAX
  char* buffer = malloc(PATH_MAX);
  if(buffer == NULL) {
  //  perror("malloc dans slash_cwd");
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

  if(PRINT_PWD)
    printf("%s\n", buffer);
  else
    strcpy(PATH, buffer);

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
    // perror("erreur dans slash_pwd");
    return 1;
}


char * slash_get_prompt() {
  // Taille maximale (sans couleurs) de 30 Caractères
  // ((10 for two colors)* 2) + 5 for status + 1 pour dollar + 25 Path + 3 espaces + 8 (\001, \002) + 1 (\0)
  char *prompt = malloc(sizeof(char) * (45+10+8));
  if(prompt == NULL) {
    perror("Error malloc slash_get_prompt");
    return NULL;
  }
  memset(prompt, '\0', 45);
  // 2. Logical path
  char * path = getenv("PWD");
  // Handle Null
  if(path == NULL) {
    free(prompt);
    return "> ";
  }
  
  //4. Formatting prompt (tmp solution)
  //[COLOR][EXIT_CODE] [PATH]$[RESET COLOR]
  char * exit_status_prompt = get_exit_status();

  strcat(prompt, "\001"); 
  strcat(prompt, get_color(exit_status));
  strcat(prompt, "\002");
  
  strcat(prompt, "[");
  strcat(prompt, exit_status_prompt);
  strcat(prompt, "]");

  strcat(prompt, "\001");
  strcat(prompt, get_color(100)); // reset color
  strcat(prompt, "\002");
  
  // Here have to go trough function to reduce path if necessary. Example 25 max size.
  char * shorter_path = get_shorter_path(path, 25);
  
  strcat(prompt, "\001");
  strcat(prompt, get_color(1000));
  strcat(prompt, "\002");
  
  if(shorter_path == NULL) {
    strcat(prompt, path);
  } else {
    strcat(prompt, shorter_path);
    free(shorter_path);
  }
  
  strcat(prompt, "\001");
  strcat(prompt, get_color(100)); // reset color
  strcat(prompt, "\002");

  strcat(prompt, "$ ");
  free(exit_status_prompt);
  return prompt;
}


//Factoriser code 
//Commenter code
//Completer le error :
//Faire les error pour setenv
//Faire plusieurs fichiers .c  


//La fonction qui gére la commande cd 
int slash_cd(char **args)
{
  int ret_stev = 0;//Il va representer la valeur de retour des appels à setenv pour la gestion d'erreurs
  const char* pwd = getenv("PWD");
  if(pwd == NULL)
    goto error;
  const char* old_pwd = getenv("OLDPWD");
  if(old_pwd == NULL)
    old_pwd = pwd; // Temporaire. A vérifier. Si on met goto error: on lance un terminal et on execute directement ./slash. Pas de OldPWD et cd impossible.
  const char* home = getenv("HOME"); // Pour utiliser goto error
  if(home == NULL)
    goto error;

  if(args[3] != NULL) // Si il y a 4 arguments dans args alors il y en a trop (cd -P ref ref2)
    printf("cd : too many arguments, try help\n");

  else if(args[2] != NULL) {//Si il y en a 3                             
    if(strcmp(args[1], "-P") != 0 && strcmp(args[1], "-L") != 0)//alors args[1] doit être -P ou -L
      printf("cd : too many arguments, try help\n");//Sinon erreur (cd ref1 ref2)

    if(strcmp(args[1], "-P") == 0)
      return slash_cd_aux('P', pwd, args[2]);//on interprete en regardant la structure physique de l'arborescence
    else
      return slash_cd_aux('L', pwd, args[2]);//on interprete de maniére logique 
  }
  else if((args[1] == NULL) || (strcmp(args[1], "-P") == 0) || (strcmp(args[1], "-L") == 0)) {//Si cd ou cd -P ou cd -L on va à home
    if(chdir(home) != 0)
      goto error;
    if(setenv("OLDPWD", pwd, 1) <0 || setenv("PWD", home, 1) < 0){
      ret_stev = -1;
      goto error;
    }
  } else if(strcmp(args[1], "-" ) == 0) {//cd - alors on va dans repertoire précédent
    if(chdir(old_pwd) != 0)//designé par old_pwd et donc 
      goto error;

    //le repertoire precedent devient pwd et pwd devient le repertoire precedent
    if(setenv("OLDPWD", pwd, 1) <0 || setenv("PWD", old_pwd, 1) <0 ){
      ret_stev = -1;
      goto error;
    }
  }
  else {
    return slash_cd_aux('L', pwd, args[1]);//par defaut, cd ref, on interprete ref de maniére logique
  }

  return 0;

  error :
    if(pwd == NULL)
      perror("La variable d'environnement PWD n'existe pas\n");
    if(home == NULL)
      perror("La variable d'environnement HOME n'existe pas\n");
    if(old_pwd == NULL)
      perror("La variable d'environnement OLDPWD n'existe pas\n");
    if(ret_stev < 0)
      perror("erreur de setenv dans slash_cd_aux");
    //error_chdir();
    perror("slash_cd ");
    return 1;
}

char*** get_tokens_paths(char** tokens) {
  int tab_capacity = 10;
  int tab_size = 0;
  char*** tab = malloc(sizeof(char**) * tab_capacity );
  if(tab ==  NULL) {
    perror("malloc failed");
    return NULL;
  }

  for(int i=0; tokens[i] != NULL; i++) {
    if(tab_size >= tab_capacity - 1) {
      char*** ptr = realloc(tab, sizeof(char**) * tab_capacity * 2);
      if(ptr == NULL) {
        perror("malloc failed");
        return NULL;
      } 
      tab_capacity*=2;
      tab = ptr;
    }

    // On vérifie si il y a un '/' à la fin. Si oui, alors on 
    // acceptera que les dossiers
    size_t token_size = strlen(tokens[i]);
    int only_dir;
    if(token_size == 0) // Impossible normalement
      only_dir = 0;
    else
      only_dir = tokens[i][token_size-1] == '/';

    // On vérifie si on doit appliquer le double joker
    if(prefix(tokens[i], "**")) {
      tab[tab_size] = total_expansion(tokens[i], only_dir);
    }
    else {
      char** cut = cut_path(tokens[i], "*");
      // [*words; *choice]
      
      char* star = strchr(tokens[i], '*');
      if(star != NULL) {
        tab[tab_size] = get_paths(cut, NULL, only_dir);
        free_double_ptr(cut);
      }
      else {
        tab[tab_size] = cut;
      }
    }
    
    if(tab[tab_size] == NULL) {
      fprintf(stderr, "une erreur est survenue dans get_tokens_path\n");
      free_triple_ptr(tab);
      return NULL;
    }

    for(int i=0;tab[tab_size][i] != NULL;i++) {
      char* tmp = tab[tab_size][i];
      tab[tab_size][i] = remove_slashes(tab[tab_size][i]);
      free(tmp);

      // Si on voulait que des dossiers, on rajoute si besoin, un '/' à la fin
      if(only_dir) {
        size_t len = strlen(tab[tab_size][i]);
        if(len == 0) {
          fprintf(stderr, "Un argument est vide. C'est impossible.\n");
          free_triple_ptr(tab);
          return NULL;
        }

        // Un '/' est déjà présent à la fin. Donc, on a rien a faire.
        if(tab[tab_size][i][len-1] == '/')
          continue;
        // Sinon, on fait un realloc et on ajoute une case avec un '/' à la fin.
        tmp = realloc(tab[tab_size][i], len + 1 + 1); // +1 pour '/' et +1 pour '\0'
        if(tmp == NULL) {
          perror("realloc");
          free_triple_ptr(tab);
          return NULL;
        }
        strcat(tmp, "/"); // "a" -> "a/"
        tab[tab_size][i] = tmp;
      }

    }

    tab_size++;
  }
  tab[tab_size] = NULL;

  // for(int j=0;tab[j] != NULL;j++) {
  //   for(int i=0;tab[j][i] != NULL;i++) {
  //     char* tmp = tab[j][i];
  //     tab[j][i] = remove_slashes(tab[j][i]);
  //     free(tmp);
  //   }
  // }

  return tab;
}