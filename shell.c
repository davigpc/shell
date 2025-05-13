#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_TOKENS 100
#define MAX_PIPES 10

// Função para dividir a linha em tokens
char **parse_line(char *line, int *background) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    int i = 0;
    char *token = strtok(line, " \t\r\n");

    *background = 0;

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            *background = 1;
        } else {
            tokens[i++] = token;
        }
        token = strtok(NULL, " \t\r\n");
    }
    tokens[i] = NULL;
    return tokens;
}

// Função para comandos internos
int run_builtin(char **args) {
    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: argumento esperado\n");
        } else {
            if (chdir(args[1]) != 0) perror("cd");
        }
        return 1;
    } else if (strcmp(args[0], "help") == 0) {
        printf("Comandos internos:\n  cd <dir>\n  help\n  exit\nUse & para background e | para pipes.\n");
        return 1;
    }

    return 0;
}

// Executa uma linha de comando com pipes
void run_pipeline(char *line) {
    char *commands[MAX_PIPES];
    int cmd_count = 0;

    // Divide a linha nos pipes
    char *cmd = strtok(line, "|");
    while (cmd != NULL && cmd_count < MAX_PIPES) {
        commands[cmd_count++] = cmd;
        cmd = strtok(NULL, "|");
    }

    int i, pipefd[2], in_fd = 0;
    pid_t pid;

    for (i = 0; i < cmd_count; ++i) {
        pipe(pipefd);

        pid = fork();
        if (pid == 0) {
            if (i < cmd_count - 1) dup2(pipefd[1], STDOUT_FILENO);
            if (i > 0) dup2(in_fd, STDIN_FILENO);

            close(pipefd[0]);
            close(pipefd[1]);

            int bg = 0;
            char **args = parse_line(commands[i], &bg);

            // Verifica se é comando interno em pipeline
            if (run_builtin(args)) {
                fprintf(stderr, "Erro: comando interno '%s' não pode ser usado em pipeline.\n", args[0]);
                exit(1);
            }

            execvp(args[0], args);
            perror("exec");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
        }

        close(pipefd[1]);
        if (i > 0) close(in_fd);
        in_fd = pipefd[0];
    }

    // Espera todos os processos do pipeline
    while (wait(NULL) > 0);
}

int main() {
    char *line;

    while (1) {
        line = readline("myshell> ");
        if (!line) break;
        if (strlen(line) == 0) continue;

        add_history(line);

        if (strchr(line, '|')) {
            run_pipeline(line);
        } else {
            int background = 0;
            char **args = parse_line(line, &background);

            if (args[0] == NULL) {
                free(args);
                continue;
            }

            if (run_builtin(args)) {
                free(args);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                execvp(args[0], args);
                perror("exec");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
            } else if (!background) {
                waitpid(pid, NULL, 0);
            }

            free(args);
        }

        free(line);
    }

    return 0;
}
