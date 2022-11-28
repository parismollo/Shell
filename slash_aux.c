#include "slash.h"




//////Les fonctions auxiliaires de slash_ ?
//A voir

void catchSignal(int signal) {
  exit_status = signal;
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



//////Les fonctions auxiliaires de slash_cd
//Mais push_string utilisé aussi ici donc ?


//real_path permet d'obtenir le chemin absolue avec les liens symboliques et propre (sans .. ou . ou ///)
char* real_path(char* p) { // Attention copier path !!!!!!!!!!!!

  char* path = malloc(strlen(p)+1);
  if(path == NULL) {
    perror("Erreur malloc real_path");
    return NULL;
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
      perror("l'accès à un élément du chemin n'est pas autorisé "); 
      break;//changer phrase
    case ELOOP : 
      perror("le chemin contient une reference circulaire (a travers un lien symbolique"); 
      break;
    case ENAMETOOLONG :
      perror("le chemin est trop long"); 
      break;
    case ENOENT : 
      perror("le dossier n'existe pas"); 
      break;
    case ENOTDIR : 
      perror("Un élément du chemin n'est pas un repertoire"); 
      break;
  }
}

//Cette fonction va se deplacer depuis le repertoire courant pwd dans le dossier args
//Au regard de la structure physique de l'arborescence si option vaut 'P' 
//et de maniére logique sinon.
int slash_cd_aux(char option, const char* pwd, char *args) {
  int ret_stev = 0;//Il va representer la valeur de retour des appels à setenv pour la gestion d'erreurs
  if(option == 'P') {
    if(chdir(args) != 0)
      goto error;
    ret_stev = setenv("OLDPWD", pwd, 1); // On met a jour OLDPWD que lorsqu'on est sûr que chdir a fonctionné
    if(ret_stev<0)
      goto error;
    PRINT_PWD = 0;//Lorsque PRINT_PWD vaut 0, la fonction slash_pwd va copié son resultat dans PATH au lieu de l'afficher
    char* tokens[2] = {"pwd", "-P"};//On recupère le chemin absolue menant au repertoire courant sans les liens symboliques
    slash_pwd(tokens);
    PRINT_PWD = 1;
    ret_stev = setenv("PWD", PATH, 1);
    if(ret_stev<0)
      goto error;
    return 0;
  }//On interprete de maniére logique 
  int ret = 0;
  char* path = malloc(strlen(pwd) + 1 + strlen(args) + 1);
  if(path == NULL) {
    goto error;
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
    goto error;
  }
  if(chdir(realpath) != 0) { // Si chdir échoue, alors on interprete le path de manière physique
    ret = slash_cd_aux('P', pwd, args);
  }
  else {
    // printf("PATH: %s\nREAL_PATH: %s\n", path, realpath);
    ret_stev = setenv("PWD", realpath, 1); // De même pour pwd
  }
  free(path);
  free(realpath);
  ret_stev = setenv("OLDPWD", pwd, 1); // On met a jour OLDPWD que lorsqu'on est sûr que chdir a fonctionné
  
  return ret;

  error :
    if(pwd == NULL)
      perror("La variable d'environnement PWD n'existe pas\n");
    if(path == NULL){
      perror("Erreur malloc dans slash_cd_aux");
    }
    if(ret_stev < 0){
      perror("erreur de setenv dans slash_cd_aux");
    }
    if(realpath == NULL){
      free(path);
      fprintf(stderr, "Erreur avec realpath dans slash_cd_aux\n");
    }
    error_chdir();
    return 1;
}