// This file should be able to understand and execute the folloxing commands.
// A) cmd > file 
// B) cmd < file
// C) cmd >| file
// D) cmd >> fic
// E) cmd 2> fic
// F) cmd 2>| fic
// G) cmd 2>> fic

// Example 1: "cat < hello.txt"
// Expected result: display hello.txt content in terminal

// Example 2: "echo hello > hello.txt"
// Expected result: write hello in hello.txt file

// Example 3: "echo hello >| hello.txt"
// Expected result: erase content and add hello, or create and add hello.

// Example 4: "echo hello >> hello.txt"
// Expected result: concat to file hello or create and add hello.

// Example 5: "ls blabla 2> hello.txt"
// Expected result: add error to hello.txt if does not exists creates else fail

// Example 6: "ls blabla 2>| hello.txt" 
// Expected result: erase content and add error, or create and add error.

// Example 7: "ls blabla 2>> hello.txt" 
// Expected result: concat to file error or create and add error.

#include "slash.h"

int arr_len = 8;
char* arr[] = {">", "<", ">|", ">>", "2>", "2>|", "2>>", "|"};
//[TODO]: struct for number_symbol assoc = {(>, 0) (<, 1) (>|, 2) (>>, 3) (2>, 4) (2>|, 5) (2>>, 6) (|, 7)}

char** extract_args(char** flat_tab, int limit) {
  char** tab = malloc(sizeof(char*) * 10);
  if(tab == NULL) {perror("malloc failed"); exit(1);}
  for(int i=0; i < limit; i++) {
    tab[i] = malloc(sizeof(char) * (strlen(flat_tab[i])+1));
    if(tab[i] == NULL) {perror("malloc failed"); exit(1);} 
    strcpy(tab[i], flat_tab[i]);
  }
  tab[limit] = NULL;
  return tab;
}

int get_assoc_int(char* str) {
  for(int i=0; i<arr_len; i++) {
    if(strcmp(str, arr[i]) == 0) {
      return i; // True
    }
  }
  return -1;
}

int is_valid_operator(char* str) {
  int len = 8;
  for(int i=0; i<len; i++) {
    if(strcmp(str, arr[i]) == 0) {
      return 1; // True
    }
  }
  return 0; // False
}

int get_red_pos(char** tab) {
  for(int i=0; tab[i] != NULL; i++) {
    if(is_valid_operator(tab[i]))
      return i;
  }
  return -1;
}

int open_and_dup(char* red_type, char** flat_tab, int red_pos) {
  int fd;
  switch (get_assoc_int(red_type)) {
    case 0:
      // >
      //printf("readtype:%s et flat_tab:%p et redpos:%d et %s\n", red_type, flat_tab, red_pos, flat_tab[red_pos + 1]);
      // printf("flat:%s\n", flat_tab[red_pos + 1]);
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_CREAT | O_EXCL, 0666);
      if(fd == -1) {
        dprintf(2, "bash: sortie: cannot overwrite existing file\n");
        return -1;
      }
      // printf("fd=%d\n", fd);
      dup2(fd, STDOUT_FILENO);
      return fd;
    case 1:
      // <
      fd = open(flat_tab[red_pos + 1], O_RDWR);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDIN_FILENO);
      return fd;
    case 2:
      // >|
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDOUT_FILENO);
      return fd;
    case 3:
      // >>
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDOUT_FILENO);
      return fd;
    case 4:
      // 2>
      //printf("readtype:%s et flat_tab:%p et redpos:%d et %s\n", red_type, flat_tab, red_pos, flat_tab[red_pos + 1]);
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_CREAT | O_EXCL, 0666);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDERR_FILENO);
      //printf("fd=%d\n", fd);
      return fd;
    case 5:
      // 2>|
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDERR_FILENO);
      return fd;
    case 6:
      // 2>>
      fd = open(flat_tab[red_pos + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
      if(fd == -1) {perror("failed to open"); return -1;}
      dup2(fd, STDERR_FILENO);
      return fd;
    default:
      return -1;
  }
}
// ls < test > t2 2> t3
void redirection(char** flat_tab) {
  int red_pos = get_red_pos(flat_tab);
  if(red_pos == -1) {
    slash_exec(flat_tab);
    return;
  }
  // Get all arguments before operator
  char** cmd_args = extract_args(flat_tab, red_pos);

  char* red_type = flat_tab[red_pos];
  
  // int saved[3] = {dup(0), dup(1), dup(2)};
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stdin = dup(STDIN_FILENO);
  int saved_stderr = dup(STDERR_FILENO);

  int fd[3] = {0};
  char** ptr = flat_tab;
  for(int i=0;i<3;i++) {
     //printf("i=%d\n", i);
     //printf("redtype %s et redpos %d et i=%d\n", red_type, red_pos, i);
    // Opening file and dup2
    fd[i] = open_and_dup(red_type, ptr, red_pos);
     //printf("redtype %s et redpos %d et i=%d\n", red_type, red_pos, i);
    // Redirection interdite
    if(fd[i] < 0) // Attention ici il faut restaurer ceux qui ont marché
    {
      //printf("ozenfoeznozenfez fze\n");
      goto reset;
    }
    //  printf("redtype %s et redpos %d et i=%d\n", red_type, red_pos, i);
    ptr = ptr + red_pos + 1;
    // printf("ptr:%p et %d %d\n", ptr, red_pos, get_red_pos(ptr));
    red_pos = get_red_pos(ptr);
    if(red_pos == -1) {
      //printf("here");
      break;
    }

    red_type = ptr[red_pos];
  }

  slash_exec(cmd_args);

  // for(int i=0;i<3;i++) {
  //   dup2(saved[i], i);
  //   close(saved[i]);
  // }

  goto reset;

  reset:
    for(int i=0;i<3;i++) {
      if(fd[i] >= 0)
        close(fd[i]);
    }
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);
    // Clean memory + Annuation dup2
    if(cmd_args)
      free_double_ptr(cmd_args);
}


// int main() {
  
//   char ** flat_tab = malloc(4 * sizeof(char*));
//   if(flat_tab == NULL) {perror("malloc failed"); exit(1);}

//   flat_tab[0] = malloc(strlen("cat")+1);
//   strcpy(flat_tab[0], "cat");
//   flat_tab[1] = malloc(strlen("<")+1);
//   strcpy(flat_tab[1], "<");
//   flat_tab[2] = malloc(strlen("hello.txt")+1);
//   strcpy(flat_tab[2], "hello.txt");
//   flat_tab[3] = NULL;
  
 
//   char ** flat_tab2 = malloc(5 * sizeof(char*));
//   if(flat_tab2 == NULL) {perror("malloc failed"); exit(1);}

//   flat_tab2[0] = malloc(strlen("ls")+1);
//   strcpy(flat_tab2[0], "ls");
//   flat_tab2[1] = malloc(strlen("blabla")+1);
//   strcpy(flat_tab2[1], "blabla");
//   flat_tab2[2] = malloc(strlen("2>>")+1);
//   strcpy(flat_tab2[2], "2>>");
//   flat_tab2[3] = malloc(strlen("new_file.txt")+1);
//   strcpy(flat_tab2[3], "new_file.txt");
//   flat_tab2[4] = NULL;
  
//   redirection(flat_tab);
//   redirection(flat_tab2);

//   // Clean up memory
//   for(size_t i = 0; i< 4; i++) {
//     free(flat_tab2[i]);
//   }
//   for(size_t i = 0; i< 3; i++) {
//     free(flat_tab[i]);
//   }
//   free(flat_tab2);
//   free(flat_tab);

//   return EXIT_SUCCESS; 
// }