#include "slash.h"

char** get_paths(char** input, char** output);
char*** get_tokens_paths(char** tokens);

int PRINT_PWD = 1; // Variable auxiliaire pour cd
char PATH[PATH_MAX];

// Colors
char *colors[] = {
  "\033[32m", // green
  "\033[91m", // red
  "\033[34m", // blue
  "\033[36m", // cyan
  "\033[00m",// white
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
    exit_status = 0;
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
  for(int i = len; i<100; i++) {tokens[i] = NULL;}
  return tokens;
}


void slash_exec(char **tokens) {
  // tokens should look like [["cd"], ["path/to/somewhere"]]
  
  // If there is nothing to execute, leave this function. 
  if(tokens[0] == NULL) return;
  
  // Test jokers expansion
  // For each token (starting from 1) run expansion and print
  // int i = 1;
  // char **results;
  // while(tokens[i] != NULL) {
  //   results = joker_expansion(tokens[i++]);
  //   int j = 0;
  //   while(results[j] != NULL) {
  //     printf("%s\n", results[j++]);
  //   }
  //   get_paths(results);
  //   free_double_ptr(results);
  // }
  
  // TESTS POUR GET_PATHS
  
  // char* input[] = {"a/*", "c/*", NULL};
  
  // char** paths = get_paths(input, NULL);
  // printf("PATHS: %p\n", paths[0]);
  // for(int i=0; paths[i] != NULL; i++) {
  //   printf("%s\n", paths[i]);
  // }
  // free_double_ptr(paths);
  

  // TESTS POUR GET_TOKENS_PATHS
  // char*** example = get_tokens_paths(tokens);
  // for(int i=0; example[i] != NULL; i++) {
  //   printf("TAB [%d]:\n", i);
  //   for(int j=0; example[i][j] != NULL; j++) {
  //     printf("Element %d: %s\n", j, example[i][j]);
  //   }
  // }

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

  

  // On vérifie si au moins 1 commande a été executé. Si ce n'est pas le cas,
  // c'est que la commande dans tokens[0] n'existe pas.
  if(no_command) {
      exit_status = 127;
      fprintf(stderr, "slash: commande introuvable\n");
      return;
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
  // 10 for two colors + 5 for status + 1 pour dollar + 25 Path + 3 espaces + 1 (\0)
  char *prompt = malloc(sizeof(char) * (45));
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
  strcat(prompt, get_color(exit_status));
  
  strcat(prompt, "[");
  strcat(prompt, exit_status_prompt);
  strcat(prompt, "]");

  strcat(prompt, get_color(100));
  
  // Here have to go trough function to reduce path if necessary. Example 25 max size.
  char * shorter_path = get_shorter_path(path, 25);
  
  if(shorter_path == NULL) {
    strcat(prompt, path);
  } else {
    strcat(prompt, shorter_path);
    free(shorter_path);
  }
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

  if(args[3] != NULL)//Si il y a 4 arguments dans args alors il y en a trop (cd -P ref ref2)
    goto error;

  else if(args[2] != NULL) {//Si il y en a 3                             
    if(strcmp(args[1], "-P") != 0 && strcmp(args[1], "-L") != 0)//alors args[1] doit être -P ou -L
      goto error;//Sinon erreur (cd ref1 ref2)

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
    if(args[3] != NULL)
      printf("cd : too many arguments, try help\n");
    if(strcmp(args[1], "-P") != 0 && strcmp(args[1], "-L") != 0)
      printf("cd : too many arguments, try help\n");
    if(pwd == NULL)
      perror("La variable d'environnement PWD n'existe pas\n");
    if(home == NULL)
      perror("La variable d'environnement HOME n'existe pas\n");
    if(old_pwd == NULL)
      perror("La variable d'environnement OLDPWD n'existe pas\n");
    if(ret_stev < 0)
      perror("erreur de setenv dans slash_cd_aux");
    error_chdir();
    return 1;
}

char** joker_expansion(char* path) {
  char* star = strchr(path, '*');
  if(star == NULL)
    return NULL;
  *star = '\0';

  char* pwd = getenv("PWD");
  if(pwd == NULL) {
    fprintf(stderr, "erreur pwd");
    return NULL;
  }

  char* new_path = malloc(strlen(pwd) + 1 + strlen(path) + 1);
  if(new_path == NULL)
    return NULL;
  if(*path == '/') {
    strcpy(new_path, path);
  }
  else {
    strcpy(new_path, pwd);
    strcat(new_path, "/");
    strcat(new_path, path);
  }

  char* realpath = real_path(new_path);
  free(new_path);
  if(realpath == NULL) {
    fprintf(stderr, "Erreur avec realpath dans joker_expansion\n");
    return NULL;
  }
  
  *star = '*';

  DIR* dir = opendir(realpath);
  // printf("OPENING folder: %s\n", realpath);
  if(dir == NULL) {
    free(realpath);
    // perror("Erreur ouverture dossier");
    // Pas de perror. Car l'expansion d'un joker peut échouer.
    // On ne veut pas pour autant afficher cette erreur
    return NULL;
  }
  
  char** list = malloc(sizeof(char*) * 5);
  if(list == NULL) {
    free(realpath);
    perror("Erreur malloc");
    return NULL;
  }
  int lsize = 5, counter = 0;

  char temp[PATH_MAX];
  struct dirent* entry;
  while((entry = readdir(dir)) != NULL) {
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    // On verifie si le nom de l'entree respect le pattern imposé par le joker
    // On verifie aussi qu'il ne s'agit pas d'un fichier ou dossier caché
    if(joker_cmp(star, entry->d_name) && entry->d_name[0] != '.') {
      if(counter+1 >= lsize) {
        char** ptr = realloc(list, lsize * 2 * sizeof(char*));
        if(ptr == NULL) {
          free(realpath);
          free_double_ptr(list);
          closedir(dir);
          perror("malloc");
          goto error;
        }
        lsize *=2;
        list = ptr; 
      }
      // Attention ici. Peut être qu'il faut vérifier si on dépasse PATH_MAX caractères.
      strcpy(temp, realpath);
      strcat(temp, "/");
      strcat(temp, entry->d_name);
      list[counter++] = real_path(temp);
    }
  }
  list[counter] = NULL;
  free(realpath);
  closedir(dir);
  return list;
  
  error:
    if(dir)
      closedir(dir);
    if(list)
      free(list);
    return NULL;
}

int joker_cmp(char* joker, char* name) {
  char* target = joker + 1;
  if(*target == '\0')
    return 1;
  char * pos = strrchr(name, *target);
  return (pos != NULL && strcmp(target, pos) == 0);
}

void free_double_ptr(char** ptr) {
  // printf("FREE: \n");
  for(int i=0;ptr[i] != NULL;i++) {
    // printf("free en cours %p %s\n", ptr+i, ptr[i]);
    free(ptr[i]);
  }
  free(ptr);
}

void free_triple_ptr(char*** ptr) {
  // printf("FREE: \n")
  for(int i=0;ptr[i] != NULL;i++) {
    // printf("free en cours %p %s\n", ptr+i, ptr[i]);
    free_double_ptr(ptr[i]);
  }
  free(ptr);
}

// (int* target_size) -> Pointeur sur int car la taille de target peut changer
char** concat(char** target, int* target_size, char** source) {
  if(target == NULL || source == NULL || target_size == NULL) {
    fprintf(stderr, "concat: target, source ou target_size égale à NULL\n");
    return NULL;
  }
  int i;
  for(i=0;target[i]!= NULL;i++)
    ;
  for(int j=0; source[j] != NULL;j++) {
    if(i >= *target_size - 1) {
      char** ptr = realloc(target, sizeof(char*) * (*target_size * 2));
      if(ptr == NULL) {
        perror("failed realloc");
        return target;
      }
      target = ptr;
      *target_size *= 2;
    }
    char* str = malloc(strlen(source[j])+1);
    if(str == NULL) {
      perror("malloc in concat");
      return target;
    }
    strcpy(str, source[j]);
    target[i++] = str;
  }
  target[i] = NULL;

  return target;
}

char** get_paths(char** input, char** output) {
  // input = ["a/*", "c/*", NULL];
  // output = NULL 

  // DEBUG OUTPUT TAB
  for(int i=0;output != NULL && output[i] != NULL;i++) {
    printf("output : %s\n", output[i]);
  }
  puts("\n");
  if(input == NULL) {
    fprintf(stderr, "get_paths: Le tableau input ne doit pas etre NULL\n");
    return NULL;
  }

  if(output == NULL) {
    if(input[0] == NULL) {
      fprintf(stderr, "get_paths: output et input sont NULL ou vides\n");
      return NULL;
    }
    
    char** aux = malloc(sizeof(char*) * 2);
    if(aux == NULL) {
      perror("malloc failed");
      return NULL;
    }
    // aux = ["", ""]
    aux[0] = NULL;
    // aux = [NULL, ""]
    char* str = malloc(strlen(input[0])+1);
    // str = "    " taille 4
    if(str == NULL) {
      perror("malloc in concat");
      free_double_ptr(aux);
      return NULL;
    }
    strcpy(str, input[0]);
    // str = "a/*"
    aux[0] = str;
    // aux = ["a/*", ""]
    aux[1] = NULL;  
    // aux = ["a/*", NULL]
    // not checked here about input+1
    return get_paths(input+1, aux);
  }

  int flat_size = 10;
  char** flat = malloc(sizeof(char*) * flat_size);
  // flat = [[],[],[], ....]
  if(flat == NULL) {
    perror("erreur malloc get_paths");
    return NULL;
  }
  *flat = NULL;
  // flat = [NULL, .....]
  for(int i=0;output[i] != NULL;i++) {
    char** expansion = joker_expansion(output[i]);
    
    if(expansion == NULL) {
      // Cela signifie que aucun fichier/dossier ne matche avec le joker output[i]
      // On passe donc au suivant, avec un continue
      continue;
    }
    flat = concat(flat, &flat_size, expansion);
    free_double_ptr(expansion);
  }