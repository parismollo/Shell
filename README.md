# Architecture 
Ce fichier présente la stratégie adoptée lors de la réalisation du projet.
Ce projet contient différents modules et utilise plusieurs structures de données.

## Les différents modules et structures de données 

Les modules sont les suivants :

- **main.c** : lit en boucle ce que l'utilisateur tape et mets à jour le prompt.

- **slash.h** : contient les bibliothèques et les déclarations des fonctions des autres fichiers.

- **slash.c** : contient les définitions des commandes internes au projet slash, notamment *slash_cd*, *slash_pwd* et *slash_exit*, ainsi que les fonctions nécessaires à la lecture, l'interprétation et l'exécution des commandes.
La structure de donnée command est composée d'un "char * name" et d'un pointeur vers une fonction "int (*function) (char** args)" dont le type de retour est un entier et qui prend en paramètre "char **args".
Cette structure de command, définie dans **slash.h**, est utilisé dans le module **slash.c** afin de savoir si la commande tapée par l'utilisateur est une commande interne au projet ou non.

- **jokers.c** : contient les définitions des fonctions permettant l'expansion des jokers demandés.

- **slash_redirections.c** : contient les définitions des fonctions permettant la gestion des redirections demandées, excepté les redirections avec un pipeline.

- **slash_pipe.c** : contient les définitions des fonctions permettant la gestion des redirections demandées avec un pipeline.

- **mystring.c** : contient des fonctions utiles pour modifier ou créer des strings. Ces fonctions sont utilisées dans les autres modules du projet.

- **slash_aux.c** : contient des fonctions auxiliaires qui vont être utilisées par les autres modules.
Tant que la valeur d'exit_loop est différente de 0, on exécute les instructions suivantes.

## Déroulement de l'algorithme de slash

### 1. On récupère et traite le commande tapée par l'utilisateur


Dans la fonction main de **main.c**, on appelle *slash_read* qui utilise *slash_get_prompt* pour obtenir le chemin actuel, puis *readline*, une fonction de la bibliothèque **readline.h**, qui va renvoyer la ligne lue dans le prompt pour mettre à jour la variable prompt_line.

Si aucune ligne n'a été lu alors on met exit_loop à 0 afin de sortir de la boucle et de terminer le programme.
Sinon, on appelle *slash_interpret* qui va vérifier grâce à *strtok* que la ligne de commande lue ne contient pas des arguments trop long ou n'a pas trop d'arguments, et ensuite renvoyer un tableau contenant les différents arguments de la ligne.

On appelle ensuite *get_tokens_path* pour récupérer le tableau des tokens. Dans cette fonction, on va aussi vérifier s'il y a des jokers à gérer et les gérer avec les fonctions de jokers *prefix*, *total_expansion*, *cut_path* et *get_paths*.
Puis on appelle *flat_triple_tab* qui permet d'aplatir un tableau à deux dimensions de char* en tableau de char* sur tokens qui contient le tableau des arguments.

### 2. Execution de la commande

### A.Gestion des redirections 

Ensuite *apply_pipes* permet d'exécuter la commande avec un pipeline si nécessaire et sinon en gérant les redirections, car on va appeler la fonction redirection dans *apply_pipes*.
La fonction redirection va gérer les redirections si nécessaire et exécuter la commande grâce à slash_exec de **slash.c**.

### B.Exécution d'une commande interne 

*slash_exec* va tester si la commande est une commande interne en la comparant avec les commandes internes dans un tableau library.
Si c'est une commande interne, on va l'exécuter avec :
- *slash_exit* qui va mettre exit_loop à 0 pour quitter la boucle du main et mettre à jour l'exit_status si nécessaire qui est la valeur renvoyée par la fonction *main* ;

- *slash_help* qui va afficher une aide pour l'utilisateur avec les différentes commandes internes et leurs options ;

- *slash_pwd* qui va afficher une référence absolue du répertoire de travail courant. Soit la référence absolue logique s'il y a l'option -L ou s'il n'y a pas d'options et sinon une référence absolue physique.
Pour la référence logique, on utilise *getenv("PWD")*.
Pour la référence physique, on va vérifier si on est à la racine avec *isroot* et sinon on va remonter jusqu'à la racine en concaténant les noms des parents à chaque fois que l'on récupère avec *get_dirname* et que l'on concatène au début avec *push_string* de **slash_aux.c**.

- *slash_cd* qui va changer de répertoire.
On utilise *slash_cd_aux* pour se déplacer de manière physique en utilisant *chdir* puis en mettant à jour OLDPWD avec *setenv* et en on récupère le chemin absolue menant au repertoire courant sans les liens symboliques avec *slash_pwd* et l'option -P pour mettre à jour PWD avec *setenv*.
On utilise aussi *slash_cd_aux* pour se déplacer de manière logique. On va construire une référence soit en prenant la référence donnée avec la commande si elle est absolue, sinon en l'ajoutant à la référence du répertoire courant obtenue avec *getenv("PWD")*.
On supprime ce qui est inutile dans cette référence (// ou /./ etc.) avec la fonction *real_path* de **slash_aux.c**
On se déplace ensuite à cette référence avec *chdir* et on mets à jour PWD avec *setenv* et la référence construite dans la fonction.
Si *chdir* échoue on appel *slash_cd_aux* avec -P pour se déplacer de manière physique comme expliquée précédemment.

### C.Exécution d'une commande externe

Si ce n'est pas une commande interne, on utilise *exec* de **slash_aux.c** qui permet d'exécuter une commande externe.
Dans main on utilise struct sigaction avec sa_handler = SIG_IGN pour ignorer les signaux SIGINT et SIGTERM. Mais ces signaux ne doivent pas être ignorés par les processus exécutant des commandes externes donc on va utiliser struct sigaction avec sa_handler = SIG_DFL pour rétablir la gestion des signaux SIGINT et SIGTERM dans les processus fils. C'est-à-dire que l'on utilise *fork* et dans le processus fils, on rétablit la gestion de ces signaux avec *sigaction* et on utilise *execvp* pour exécuter la commande externe.
Dans le père, on attend que le processus fils soit terminer et on met à jour exit_status avec WEXITSTATUS.

### 3. Free, exit ou nouvelle commande

On libére ensuite les variables avec *free_double_ptr* et *free_triple_ptr* de **slash_aux.c** et *free* et on revient au début de la boucle.
Si la valeur d'exit_loop est 0, alors la condition de boucle n'est plus vérifiée donc on quitte la boucle.
On appelle *rl_clear_history* qui va supprimer l'historique de *readline* et on retourne la valeur de l'exit_status. 
