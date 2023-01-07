#include "slash.h"

/* Expansion d'un path contenant une seule et unique étoile */
char** joker_expansion(char* path, int only_dir) {
  char* star = strchr(path, '*');
  if(star == NULL) {
    // On vérifie si le fichier ou dossier existe.
    // Si oui on le renvoie dans un tableau 2 cases (NULL a la fin)
    // Sinon on renvoie NULL
    if(!file_exists(path, only_dir)) {
      return NULL;
    }
    // On fait un tableau char** avec juste le path
    char** no_joker = malloc(sizeof(char*) * 2);
    if(no_joker == NULL) {
      return NULL;
    }
    char* tmp = malloc(strlen(path) + 1);
    if(tmp == NULL) {
      free(no_joker);
      return NULL;
    }
    strcpy(tmp, path);
    no_joker[0] = tmp;
    no_joker[1] = NULL;
    return no_joker;
  }
  *star = '\0';

  char* new_path = malloc(strlen(path) + 1);
  if(new_path == NULL)
    return NULL;
  // words/
  strcpy(new_path, path);
  
  *star = '*';

  DIR* dir;
  
  if(*new_path == '\0') {
    dir = opendir(".");
  } 
  else {
    dir = opendir(new_path);
  }
  // printf("OPENING folder: %s\n", realpath);
  if(dir == NULL) {
    free(new_path);
    // perror("Erreur ouverture dossier");
    // Pas de perror. Car l'expansion d'un joker peut échouer.
    // On ne veut pas pour autant afficher cette erreur
    return NULL;
  }
  
  char** list = malloc(sizeof(char*) * 5);
  if(list == NULL) {
    free(new_path);
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
    //printf("star: %s, entry->name: %s, %ld, %ld\n", star, entry->d_name, strlen(star), strlen(entry->d_name));
    if(joker_cmp(star, entry->d_name) && entry->d_name[0] != '.') {
      
      if(counter+1 >= lsize) {
        char** ptr = realloc(list, lsize * 2 * sizeof(char*));
        if(ptr == NULL) {
          free(new_path); //ici
          free_double_ptr(list);
          closedir(dir);
          perror("malloc");
          goto error;
        }
        lsize *=2;
        list = ptr; 
      }

      // Attention ici. Peut être qu'il faut vérifier si on dépasse PATH_MAX caractères.
      strcpy(temp, new_path); //ici
      // if(*new_path != '\0') {
      //   strcat(temp, "/");
      // }
      strcat(temp, entry->d_name);

      // Si on accepte uniquement les dossiers et que ce n'est pas un dossier
      // Alors on ajoute pas le path à la liste
      if(only_dir && !file_exists(temp, only_dir))
        continue;

      char* temp2 = malloc(strlen(temp) + 1);
      if(temp2 == NULL) {
        free(new_path);
        free_double_ptr(list);
        closedir(dir);
        perror("malloc");
        goto error;
      }
      strcpy(temp2, temp);

      list[counter++] = temp2;
    }
  }
  list[counter] = NULL;
  free(new_path);//ici
  closedir(dir);
  return list;
  
  error:
    if(dir)
      closedir(dir);
    if(list)
      free(list);
    return NULL;
}

/* Vérifie si un path avec une '*' matche avec la string name */
int joker_cmp(char* joker, char* name) {
  char* target = joker + 1;
  if(*target == '\0')
    return 1;
  // char * pos = strchr(name, *target);
  // *ad
  // _aaaad
  int j = strlen(name) -1;
  int i = strlen(joker)-1;
  for(; joker[i] != '*' && i >= 0 && j >= 0; i--, j--) {  
      if(joker[i] != name[j]) {
        return false;
      }
  }
  if(j < 0 && joker[i] != '*') {
    return false;
  }
  return true;
}

// Expansion avec joker_expansion de chaque case du tableau input auxquelle
// on concatène la précédente. On developpe donc étoile par étoile et on obtient
// une multitude de paths:
// [a/*, *.c] -> [a/c/file.c, a/c/file2.c, a/b/h.c] 
char** get_paths(char** input, char** output, int only_dir) {
  // input = ["a/*", "c/*", NULL];
  // output = NULL 

  //DEBUG OUTPUT TAB
  // for(int i=0;output != NULL && output[i] != NULL;i++) {
  //   printf("output : %s\n", output[i]);
  // }
  // puts("\n");
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
    return get_paths(input+1, aux, only_dir);
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
    char** expansion = joker_expansion(output[i], only_dir);
    if(expansion == NULL) {
      // Cela signifie que aucun fichier/dossier ne matche avec le joker output[i]
      // On passe donc au suivant, avec un continue
      continue;
    }
    flat = concat(flat, &flat_size, expansion);
    free_double_ptr(expansion);
  }
  free_double_ptr(output);
  if(input[0] == NULL) {
    return flat;
  }

  for(int i=0;flat[i] != NULL; i++) {
    char* ptr = realloc(flat[i], strlen(flat[i]) + 1 + strlen(input[0]) + 1);
    if(ptr == NULL) {
      free_double_ptr(flat);
      return NULL;
    }
    flat[i] = ptr;
    strcat(flat[i], "/");
    strcat(flat[i], input[0]); 
  }

  return get_paths(input+1, flat, only_dir);
}

// Transforme un path avec plusieurs étoile en tableau de string
// avec l'étoile comme delimiter. Ex: a/*/c/d/*.c -> [a/*, c/d/*.c]
char** cut_path(char* path, char* delim) {
  int size = 10, len = 0;
  char* old_path = path, *tmp = NULL, *star = NULL, *end = NULL,
  *token = NULL, *slash = NULL;
  char** sub_paths = NULL;
  // Le tableau qui va contenir les morceaux du path (coupé après chaque '*')
  sub_paths = malloc(sizeof(char*) * size + 1);
  if(sub_paths == NULL)
    goto error;
  // En cas de goto error et de free_double_ptr(sub_paths)
  // il faut absolument qu'il y est un NULL toujours dans la derniere case
  sub_paths[0] = NULL;

  star = strchr(path, '*');
  if(star == NULL) {
    tmp = malloc(strlen(path) + 1);
    if(tmp == NULL)
      goto error;
    strcpy(tmp, path);
    sub_paths[0] = tmp;
    sub_paths[1] = NULL;
    return sub_paths;
  }

  // On fait une copie de path
  tmp = malloc(strlen(path) + 2); // +2 pour ' ' et '\0'
  if(tmp == NULL)
    goto error;
  // On ajoute un espace en plus pour éviter un bug si
  // le path commence par une étoile
  strcpy(tmp, " ");
  strcat(tmp, path);
  path = tmp;

  end = strchr(path, '\0'); // Ne peux pas échouer
  token = strtok(path, delim);
  
  if(token == NULL) {
    tmp = copy_str(old_path);
    if(tmp == NULL)
      goto error;
    sub_paths[0] = tmp;
    sub_paths[1] = NULL;
    return sub_paths;
  }
  
  tmp = malloc(strlen(token) + 2); // +2 car taille pour '*' et '\0'
  if(tmp == NULL)
    goto error;
  strcpy(tmp, token);
  strcat(tmp, "*");

  while(token != NULL) {
    if(len >= size - 1) {
      char** ptr = realloc(sub_paths, size * 2 * sizeof(char*));
      if(ptr == NULL)
        goto error;
      size *= 2;
      sub_paths = ptr;
    }
    
    if(len > 0) {
      slash = strchr(token, '/');
      // si il y a un slash
      if(slash != NULL) {
        // C'est ici qu'est la fin du mot
        *slash = '\0';
        
        // On copie le bout avant le '/' sur le mot d'avant
        tmp = realloc(sub_paths[len-1], strlen(sub_paths[len-1]) + strlen(token) + 1);
        if(tmp == NULL)
          goto error;
        strcat(tmp, token);
        sub_paths[len-1] = tmp;

        // Puis, on copie le bout après le slash avec une étoile en plus
        // Pour ça, il suffit de dire que token = slash
        token = slash + 1;
        if(token == end) {
          break;
        }
      }
      else {
        tmp = realloc(sub_paths[len-1], strlen(sub_paths[len-1]) + strlen(token) + 1);
        if(tmp == NULL)
          goto error;
        strcat(tmp, token);
        sub_paths[len-1] = tmp;
        break;
      }

      tmp = malloc(strlen(token) + 2);
      if(tmp == NULL)
        goto error;
      strcpy(tmp, token);
      strcat(tmp, "*");
    }

    sub_paths[len++] = tmp;
    // Pour ne pas qu'il y est de crash en cas d'appel de free_double_ptr
    sub_paths[len] = NULL;
    token = strtok(NULL, delim);
  }
  
  // On supprime l'espace rajouté au debut et qui est dans le premier token
  size_t new_size = strlen(sub_paths[0]) - 1;
  memmove(sub_paths[0], sub_paths[0] + 1, new_size);
  sub_paths[0][new_size] = '\0';

  if(len > 0) {
    char* word = sub_paths[len-1];
    size_t len_word = strlen(word);
    if(len_word > 1 && word[len_word-1] == '*' && word[len_word-2] != '/')
      word[len_word-1] = '\0';
  }

  // Un NULL est toujours présent à la fin donc cette ligne
  // n'est plus necessaire.
  // sub_paths[len] = NULL;
  free(path);
  return sub_paths;

  error:
    perror("Error malloc or realloc in cut_paths");
    if(sub_paths)
      free_double_ptr(sub_paths);
    if(tmp)
      free(tmp);
    if(path && path != old_path)
      free(path);
    return NULL;
}

/* Vérifie si pre est bien préfix de str */
int prefix(char* str, char* pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

//// DEBUT JOKER DOUBLE "**" ////

/* Expansion de l'étoile double "**" */
// On considère que le path doit toujours commencer par "**" ou "**/"
char** total_expansion(char* path, int only_dir) {
  size_t path_size = strlen(path);
  if(path_size < 2 || (!prefix(path, "**") && !prefix(path, "**/")))
    return NULL;

  int len = 0, cap = 10;
  char*** exp = malloc(sizeof(char**) * cap);
  if(exp == NULL) {
    perror("error malloc total_expansion");
    return NULL;
  }
  exp[0] = NULL;

  string* dir = string_new(100);
  if(dir == NULL) {
    perror("erreur creation d'un struct string total_expansion");
    free(exp);
    return NULL;
  }
  
  char* no_slashes = remove_slashes(path);
  if(no_slashes == NULL) {
    fprintf(stderr, "Error remove slashes total_expansion");
    free(exp);
    string_delete(dir);
    return NULL;
  }
  size_t no_slashes_size = strlen(no_slashes);

  // (devenu inutile car on passe deja cette variable en argument de la fonction)
  // Si le path termine par un '/', alors on cherche uniquement les dossiers
  // int only_dir = no_slashes[no_slashes_size - 1] == '/'; // no_slashes_size toujours > 0 ici

  // Si on a juste '**' ou '**/' on considère que le pattern est '*' (on cherche tous fichiers dans l'arbo courante)
  // Sinon on prend ce qu'il y a pres le '**/'
  char* pattern = no_slashes_size > 3 ? no_slashes + 3 : "*";

  char*** paths = total_expansion_aux(dir, pattern, only_dir, exp, &len, &cap);
  free(no_slashes);
  string_delete(dir);
  
  if(paths != NULL) {
    char** flat_paths = flat_triple_tab(paths);
    free_triple_ptr(paths);
    return flat_paths;
  }
  free_triple_ptr(exp);
  return NULL;
}

/* Fonction auxiliaire pour l'expansion de l'étoile double */
// On recherche "pattern" dans tous les dossiers en partant du dossier "path"
// On ajoute chaque résultat dans le tableau de strings "exp"
char*** total_expansion_aux(string* path, char* pattern, int only_dir, char*** exp, int* size, int* cap) {
  // On vérifie si exp à une taille suffisante
  if(*size >= *cap - 1) {
    char*** ptr = realloc(exp, sizeof(char**) * (*cap) * 2);
    if(ptr == NULL) {
      perror("malloc failed");
      return NULL;
    }
    *cap *=  2;
    exp = ptr;
  }

  // On commence par ajouter les paths qui matchent avec le pattern
  // En partant du dossier path
  
  // On concatène à path le pattern (temporairement)
  string_append(path, pattern);

  char** cut = cut_path(path->data, "*");
  // ex: [*words; *choice]
  char* star = strchr(path->data, '*');
 
  if(star != NULL) {
    exp[*size] = get_paths(cut, NULL, only_dir);
    free_double_ptr(cut);
  }
  else {
    exp[*size] = cut;
  }
  (*size)++;
  exp[*size] = NULL;

  // On retire le pattern de notre string path
  string_truncate(path, strlen(pattern));

  DIR * dir = NULL;
  struct dirent* entry;
  struct stat st;

  // On ouvre le dossier path
  if(path->data[0] == '\0')
    dir = opendir(".");
  else
    dir = opendir(path->data);
  if(dir == NULL) {
    fprintf(stderr, "Impossible d'ouvrir le dossier %s\n", path->data);
    goto error;
  }
  
  while((entry = readdir(dir)) != NULL) {
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
  
    // si l'entrée considérée est un répertoire, on y poursuit
    // récursivement la recherche 
    if(fstatat(dirfd(dir), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0) {
      perror("ERROR fstatat dans total_expansion_aux");
      goto error;
    }

    if(S_ISDIR(st.st_mode) && entry->d_name[0] != '.') {
      // On concatène le nom du dossier sur notre string
      string_append(path, entry->d_name);
      string_append(path, "/");
      char*** tmp_exp = total_expansion_aux(path, pattern, only_dir, exp, size, cap);
      if(tmp_exp == NULL)
        goto error;
      exp = tmp_exp;
      string_truncate(path, 1 + strlen(entry->d_name));
    }
  }

  closedir(dir);
  return exp;

  error:
    if(dir)
      closedir(dir);
    if(exp)
      free_triple_ptr(exp);
    return NULL;
}