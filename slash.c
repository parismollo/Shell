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
#include "slash.h"
#include <signal.h>


// Static functions
static char * get_color(int n);
static char * get_exit_status();
static char * get_shorter_path(char * string, int max_size);
static void catchSignal(int signal); 

static int PRINT_PWD = 1; // Variable auxiliaire pour cd
static char PATH[PATH_MAX];

// Colors
static char *colors[] = {
  "\033[32m", // green
  "\033[91m", // red
  "\033[34m", // blue
  "\033[36m", // cyan
  "\033[00m",// white
};

// variables
static char *prompt_line = NULL;
static int exec_loop   = 1, // Variable pour sortir du while dans le main
           exit_status = 0; // Par défaut, une valeur de sortie de 0
static command library[] = {
  {"exit", slash_exit},
  {"pwd",  slash_pwd},
  {"help", slash_help},
  {"cd", slash_cd}
};

void catchSignal(int signal) {
  exit_status = signal;
}

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

int main() {
  signal(SIGINT, catchSignal);
  while(exec_loop) {
    // Step 1: Read input and update prompt line variable:
    slash_read();
    
    if(prompt_line != NULL) {
      // Step 2: Interpret input:
      char **tokens = slash_interpret(prompt_line);
      // Step 3: Execute input:
      if(tokens) {slash_exec(tokens);}
      free(tokens);
    }
    
    free(prompt_line);
  } 
  // Clear readline history:
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
        // perror("descripteur de fichier");
        return -1;
    }
    if(fstat(fd, &st) == -1) {
        // perror("erreur fstat");
        return -1;
    }
    ino_t ino = st.st_ino;
    dev_t dev = st.st_dev;

    if(lstat("/", &st) == -1) {
        // perror("erreur lstat dans is_root");
        return -1;
    }

    return st.st_ino == ino && st.st_dev == dev;
}

// Récuperer le nom d'un dossier grâce à son parent
char* get_dirname(DIR* dir, DIR* parent) {
  struct dirent *entry;
  struct stat st;

  if(fstat(dirfd(dir), &st) < 0) {
    // perror("erreur fstat dans get_dirname");
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
	    // perror("erreur stat dans get_dirname");
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
    // perror("Pas assez de place dans le buffer");
    exit_status = 1;
    return exit_status;
  }
  memmove(buffer + s1 + 1, buffer, buf_size); // On decalle ce qu'il y a dans buffer vers la droite
  buffer[0] = '/'; // On met un / au debut
  memmove(buffer + 1, str, s1); // On ecrit après le / le nom du dossier(str) dans le buffer
  exit_status = 0;
  return exit_status;
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
  }else {
    strcat(prompt, shorter_path);
    free(shorter_path);
  }
  strcat(prompt, "$ ");
  free(exit_status_prompt);
  return prompt;
}

char * get_shorter_path(char * string, int max_path_size) {
  // Get len of current path:
  int path_len = strlen(string);
  // How many chars to remove + 3 dots to add (. . .)
  int chars_to_rmv =  (path_len - max_path_size) + 3;
  // If we have to remove something then:
  if(chars_to_rmv > 0) {
    // max_path_size = 25
    // e.g if path_len = 30,  chars_to_rm = 8
    char *new_path = malloc(max_path_size + 1);
    if(new_path != NULL) {
      // add . . .
      // add last path_len - chars_to_rmv
      strcpy(new_path, "...");
      strcpy(new_path+3, string + chars_to_rmv);
      return new_path;
    }
  }
  return NULL;
}

char * get_exit_status() {
  char * str = malloc(sizeof(char) * 4);
  switch (exit_status) {
    case 2:
      strcpy(str,"SIG");
      break;
    default:
      sprintf(str, "%d", exit_status);
      break;
  }
  return str;
}

char * get_color(int n) {
  // Temporary solution
  switch (n) {
  case 0:
    return colors[0];
  case 1:
  case 2:
  case 127:
    return colors[1];
  default:
    return colors[4];
  }
}

    //Factoriser code 
    //Commenter code
    //Completer le error :
    //Faire les error pour setenv
    //Faire plusieurs fichiers .c  

//real_path permet d'obtenir le chemin absolue avec les liens symboliques et propre (sans .. ou . ou ///)
char* real_path(char* p) { // Attention copier path !!!!!!!!!!!!

  char* path = malloc(strlen(p)+1);
  if(path == NULL) {
    // perror
    exit(1);
  }
  strcpy(path, p);

  char* buf1 = malloc(PATH_MAX);
  if(buf1 == NULL) {
    perror("Erreur malloc real_path");
    free(path);
    return NULL;
  }
  memset(buf1, 0, PATH_MAX);

  char* ptr = strtok(path, "/");
  while(ptr != NULL) {
    push_string(buf1, ptr);
    ptr = strtok(NULL, "/");
  } // On a ptr qui est path mais inversé : /rep1/rep2 devient /rep2/rep1
  free(path);
  // printf("BUFFER 1 : %s\n", buf1);

  char* buf2 = malloc(PATH_MAX);
  if(buf2 == NULL) {
    perror("Erreur malloc real_path");
    free(buf1);
    return NULL;
  }
  memset(buf2, 0, PATH_MAX);

  int count = 0;
  ptr = strtok(buf1, "/"); // On separe la chaine ptr selon les "/"
  while(ptr != NULL) { // On regarde chaque element
    if(strcmp(ptr, ".") == 0)//Si c'est "." on ne l'ajoute pas à buf2
      ;
    else if(strcmp(ptr, "..") == 0) {//Si c'est ".." on ne l'ajoute pas
      count++;//et on retient que l'on ne doit pas ajouter le prochaine element different de "." et ".."
    } else {
      if(count > 0) {//On ignore l'element car il y avait ".." dans le chemin absolue après lui (et donc avant dans ptr)
        count--;
      } else {
        push_string(buf2, ptr);//On ajoute l'element à buf2
      }
    }
    
    ptr = strtok(NULL, "/");
  }
  free(buf1);

  if(buf2[0] != '/')
    buf2[0] = '/';//buf2 est un chemin absolue
  // printf("BUFFER 2 : %s\n", buf2);
  return buf2;//buf2 designe donc le chemin absolue menant au repertoire courant, contenant les liens symboliques
}

void error_chdir(){//Certaines des erreurs les plus courante lorsqu'on utilise chdir
  switch(errno){
    case EACCES : 
      perror("unauthorized access for one element of the path"); 
      break;//changer phrase
    case ELOOP : 
      perror("contient une ref circulaire (a travers un lien symbolique"); 
      break;
    case ENAMETOOLONG :
      perror("path trop long"); 
      break;
    case ENOENT : 
      perror("fichier n'existe pas"); 
      break;
    case ENOTDIR : 
      perror("Un élément de path n'est pas un repertoire"); 
      break;
  }
}

//Cette fonction va se deplacer depuis le repertoire courant pwd dans le dossier args
//Au regard de la structure physique de l'arborescence si option vaut 'P' 
//et de maniére logique sinon.
int slash_cd_aux(char option, const char* pwd, char *args) {
  if(option == 'P') {
    if(chdir(args) != 0)
      goto error;
    setenv("OLDPWD", pwd, 1); // On met a jour OLDPWD que lorsqu'on est sûr que chdir a fonctionné
    PRINT_PWD = 0;//Lorsque PRINT_PWD vaut 0, la fonction slash_pwd va copié son resultat dans PATH au lieu de l'afficher
    char* tokens[2] = {"pwd", "-P"};//On recupère le chemin absolue menant au repertoire courant sans les liens symboliques
    slash_pwd(tokens);
    PRINT_PWD = 1;
    setenv("PWD", PATH, 1);
    return 0;
  }//On interprete de maniére logique 
  int ret = 0;
  char* path = malloc(strlen(pwd) + 1 + strlen(args) + 1);
  if(path == NULL) {
    perror("Erreur malloc slash_cd_aux");
    return 1;
  }
  if(*args == '/') {
    strcpy(path, args);//Si c'est une ref absolue
  }
  else {
    strcpy(path, pwd);
    strcat(path, "/");
    strcat(path, args);//On ajoute args à pwd
  }
  char* realpath = real_path(path);//realpath va supprimer ce qui est inutile dans path ("..", "."...)
  if(realpath == NULL) {
    free(path);
    fprintf(stderr, "Erreur avec realpath dans slash_cd_aux\n");
    return 1;
  }
  if(chdir(realpath) != 0) { // Si chdir échoue, alors on interprete le path de manière physique
    ret = slash_cd_aux('P', pwd, args);
  }
  else {
    // printf("PATH: %s\nREAL_PATH: %s\n", path, realpath);
    setenv("PWD", realpath, 1); // De même pour pwd
  }
  free(path);
  free(realpath);
  setenv("OLDPWD", pwd, 1); // On met a jour OLDPWD que lorsqu'on est sûr que chdir a fonctionné
  
  return ret;

  error :
    if(pwd == NULL)
      perror("La variable d'environnement PWD n'existe pas\n");
    error_chdir();
    return 1;
}

//La fonction qui gére la commande cd 
int slash_cd(char **args)
{

  const char* pwd = getenv("PWD");
  if(pwd == NULL)
    goto error;
  const char* old_pwd = getenv("OLDPWD");
  if(old_pwd == NULL)
    old_pwd = pwd; // Temporaire. A vérifier. Si on met goto error: on lance un terminal et on execute directement ./slash. Pas de OldPWD et cd impossible.
  const char* home = getenv("HOME"); // Pour utiliser goto error
  if(home == NULL)
    goto error;

  if(args[3] != NULL) {//Si il y a 4 arguments dans args alors il y en a trop (cd -P ref ref2)
    printf("cd : too many arguments, try help\n"); // A mettre dans error
    return 1;
  }
  else if(args[2] != NULL) {//Si il y en a 3                             
    if(strcmp(args[1], "-P") != 0 && strcmp(args[1], "-L") != 0) {//alors args[1] doit être -P ou -L
      printf("cd : too many arguments, try help\n");//Sinon erreur (cd ref1 ref2)
      return 1;
    }

    if(strcmp(args[1], "-P") == 0)
      return slash_cd_aux('P', pwd, args[2]);//on interprete en regardant la structure physique de l'arborescence
    else
      return slash_cd_aux('L', pwd, args[2]);//on interprete de maniére logique 
  }
  else if((args[1] == NULL) || (strcmp(args[1], "-P") == 0) || (strcmp(args[1], "-L") == 0)) {//Si cd ou cd -P ou cd -L on va à home
    if(chdir(home) != 0)
      goto error;
    setenv("OLDPWD", pwd, 1);
    setenv("PWD", home, 1);
  } else if(strcmp(args[1], "-" ) == 0) {//cd - alors on va dans repertoire précédent
    if(chdir(old_pwd) != 0)//designé par old_pwd et donc 
      goto error;
    setenv("OLDPWD", pwd, 1);//le repertoire precedent devient pwd
    setenv("PWD", old_pwd, 1);//et pwd devient le repertoire precedent
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
    error_chdir();
    return 1;
}