#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

typedef struct {
    pid_t pid;
    char *cmd;
} child_info;

// Simple tokenizer to split a command string into argv[]
// Caller must free the returned array
char **split_command(const char *cmd) {
    // Duplicate string to tokenize safely
    char *copy = strdup(cmd);
    if (!copy) return NULL;

    int bufsize = 8;
    int argc = 0;
    char **argv = malloc(bufsize * sizeof(char *));
    if (!argv) {
        free(copy);
        return NULL;
    }

    char *token = strtok(copy, " \t");
    while (token) {
        if (argc >= bufsize - 1) {
            bufsize *= 2;
            argv = realloc(argv, bufsize * sizeof(char *));
            if (!argv) {
                free(copy);
                return NULL;
            }
        }
        argv[argc++] = strdup(token);
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    free(copy);
    return argv;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s \"<command1>\" \"<command2>\" ... \"<commandN>\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = argc - 1;
    child_info *children = malloc(n * sizeof(child_info));
    if (!children) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Create N child processes
    for (int i = 0; i < n; i++) {
        char **args = split_command(argv[i + 1]);
        if (!args) {
            fprintf(stderr, "Failed to parse command: %s\n", argv[i + 1]);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Child executes the command with arguments
            execvp(args[0], args);
            // If exec fails
            fprintf(stderr, "Failed to exec %s: %s\n", args[0], strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            // Parent remembers child
            children[i].pid = pid;
            children[i].cmd = argv[i + 1];
        }

        // Free only the array, not the child’s copy
        for (int j = 0; args[j]; j++) free(args[j]);
        free(args);
    }

    // Parent waits for children
    int remaining = n;
    while (remaining > 0) {
        int status;
        pid_t done = wait(&status);

        if (done < 0) {
            perror("wait");
            free(children);
            exit(EXIT_FAILURE);
        }

        // Identify which command finished
        for (int i = 0; i < n; i++) {
            if (children[i].pid == done) {
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("Command \"%s\" has completed successfully\n", children[i].cmd);
                } else {
                    printf("Command \"%s\" has not completed successfully\n", children[i].cmd);
                }
                break;
            }
        }
        remaining--;
    }

    printf("All done, bye-bye!\n");
    free(children);
    return 0;
}
