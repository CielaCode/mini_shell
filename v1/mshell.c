#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

// Version statiquement liée pour être vraiment indépendante
// Compilation: gcc -static -o minishell minishell.c

int main(int argc, char *argv[], char *envp[]) {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char *path = "/bin:/usr/bin";  // PATH par défaut si pas défini
    
    // Pas de prompt fancy, juste l'essentiel
    printf("Minishell autonome v1.0\n");
    printf("Tapez 'exit' pour quitter\n\n");
    
    while (1) {
        printf("$ ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) continue;
        
        // Parsing basique
        int i = 0;
        args[i] = strtok(input, " ");
        while (args[i] && i < MAX_ARGS-1) {
            args[++i] = strtok(NULL, " ");
        }
        
        if (strcmp(args[0], "exit") == 0) break;
        
        if (strcmp(args[0], "cd") == 0) {
            if (args[1]) chdir(args[1]);
            continue;
        }
        
        // Fork et exec sans chercher dans PATH
        pid_t pid = fork();
        
        if (pid == 0) {
            // Essayer différents chemins
            char *paths[] = {"/bin/", "/usr/bin/", NULL};
            int found = 0;
            
            for (int j = 0; paths[j]; j++) {
                char fullpath[256];
                snprintf(fullpath, sizeof(fullpath), "%s%s", paths[j], args[0]);
                execve(fullpath, args, envp);
            }
            
            // Si on arrive ici, échec
            printf("%s: commande introuvable\n", args[0]);
            exit(1);
        } else {
            wait(NULL);
        }
    }
    
    return 0;
}
