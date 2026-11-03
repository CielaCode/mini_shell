#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

char history[MAX_HISTORY][MAX_INPUT];
int history_count = 0;

// Gestionnaire de signal pour éviter les processus zombies
void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Ajouter une commande à l'historique
void add_to_history(const char *cmd) {
    if (strlen(cmd) == 0) return;
    
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count++], cmd);
    } else {
        // Décaler l'historique
        for (int i = 1; i < MAX_HISTORY; i++) {
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[MAX_HISTORY-1], cmd);
    }
}

// Afficher l'historique
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%5d  %s\n", i+1, history[i]);
    }
}

// Exécuter une commande externe
int execute_command(char **args) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Processus enfant
        // Rediriger les signaux vers le comportement par défaut
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        
        // Exécuter la commande
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "minishell: %s: commande introuvable\n", args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
        return 1;
    } else {
        // Processus parent
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    return 0;
}

// Obtenir le prompt coloré
void get_prompt(char *prompt, size_t size) {
    char hostname[1024];
    char cwd[1024];
    struct passwd *pw = getpwuid(getuid());
    
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));
    
    // Remplacer le home par ~
    if (pw) {
        char *home = pw->pw_dir;
        int home_len = strlen(home);
        if (strncmp(cwd, home, home_len) == 0) {
            char temp[1024];
            snprintf(temp, sizeof(temp), "~%s", cwd + home_len);
            snprintf(prompt, size, "\001\033[1;32m\002%s@%s\001\033[0m\002:\001\033[1;34m\002%s\001\033[0m\002$ ",
                     pw->pw_name, hostname, temp);
            return;
        }
    }
    
    snprintf(prompt, size, "\001\033[1;32m\002%s@%s\001\033[0m\002:\001\033[1;34m\002%s\001\033[0m\002$ ",
             pw ? pw->pw_name : "user", hostname, cwd);
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char prompt[MAX_INPUT + 50];
    
    // Configurer le gestionnaire de signal
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C dans le parent
    
    printf("\033[1;36m");  // Cyan
    printf("╔════════════════════════════════════╗\n");
    printf("║     MiniShell - Terminal simple    ║\n");
    printf("║     Tapez 'exit' pour quitter      ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("\033[0m\n");
    
    while (1) {
        // Afficher le prompt
        get_prompt(prompt, sizeof(prompt));
        printf("%s", prompt);
        fflush(stdout);
        
        // Lire l'entrée utilisateur
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        
        // Enlever le newline
        input[strcspn(input, "\n")] = 0;
        
        // Ignorer les lignes vides
        if (strlen(input) == 0) {
            continue;
        }
        
        // Ajouter à l'historique
        add_to_history(input);
        
        // Découper la commande et ses arguments
        int arg_count = 0;
        char *token = strtok(input, " ");
        
        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;
        
        if (arg_count == 0) continue;
        
        // Commandes internes
        if (strcmp(args[0], "exit") == 0) {
            printf("Au revoir !\n");
            break;
        }
        else if (strcmp(args[0], "cd") == 0) {
            const char *path = args[1] ? args[1] : getenv("HOME");
            if (chdir(path) != 0) {
                perror("cd");
            }
        }
        else if (strcmp(args[0], "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd))) {
                printf("%s\n", cwd);
            }
        }
        else if (strcmp(args[0], "clear") == 0) {
            printf("\033[2J\033[H");  // Clear screen ANSI
        }
        else if (strcmp(args[0], "history") == 0) {
            show_history();
        }
        else if (strcmp(args[0], "echo") == 0) {
            for (int i = 1; i < arg_count; i++) {
                printf("%s%c", args[i], (i < arg_count-1) ? ' ' : '\n');
            }
            if (arg_count == 1) printf("\n");
        }
        else {
            // Commande externe
            execute_command(args);
        }
    }
    
    return 0;
}
