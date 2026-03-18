#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "token.h"
#include "command.h"

#define MAXLINE 1000

static int is_pipe_sep(const char *s)  { return s && strcmp(s, "|") == 0; }
static int is_bg_sep(const char *s)    { return s && strcmp(s, "&") == 0; }
// ';' or NULL -> wait

// Execute a pipeline group: commands[i..j] (inclusive).
// If bg==1, do not wait; otherwise wait for all children.
static void run_pipeline(char *tokens[], Command *commands, int i, int j, int bg) {
    int n = j - i + 1;
    if (n <= 0) return;

    int pipes_needed = (n > 1) ? (n - 1) : 0;
    int pipes[64][2]; // enough for simple cases; feel free to make dynamic
    if (pipes_needed > 64) {
        fprintf(stderr, "Too many pipeline segments\n");
        return;
    }

    for (int k = 0; k < pipes_needed; k++) {
        if (pipe(pipes[k]) < 0) {
            perror("pipe");
            return;
        }
    }

    pid_t pids[128];
    if (n > (int)(sizeof pids / sizeof pids[0])) {
        fprintf(stderr, "Too many commands in pipeline\n");
        // Close any opened pipes
        for (int k = 0; k < pipes_needed; k++) { close(pipes[k][0]); close(pipes[k][1]); }
        return;
    }

    for (int idx = 0; idx < n; idx++) {
        int ci = i + idx;

        // Build argv for this command from tokens[first..last]
        int argc = commands[ci].last - commands[ci].first + 1;
        if (argc <= 0) continue;

        char **argv = calloc((size_t)argc + 1, sizeof(char*));
        if (!argv) {
            perror("calloc");
            // Close pipes before returning
            for (int k = 0; k < pipes_needed; k++) { close(pipes[k][0]); close(pipes[k][1]); }
            return;
        }
        for (int t = 0; t < argc; t++) {
            argv[t] = tokens[commands[ci].first + t];
        }
        argv[argc] = NULL;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(argv);
            // Parent: close pipes and bail
            for (int k = 0; k < pipes_needed; k++) { close(pipes[k][0]); close(pipes[k][1]); }
            return;
        }

        if (pid == 0) {
            // Child
            // If not first, hook stdin to previous pipe's read end
            if (idx > 0) {
                if (dup2(pipes[idx - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    _exit(127);
                }
            }
            // If not last, hook stdout to this pipe's write end
            if (idx < n - 1) {
                if (dup2(pipes[idx][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    _exit(127);
                }
            }

            // Close all pipe fds in child
            for (int k = 0; k < pipes_needed; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            execvp(argv[0], argv);
            perror("execvp");
            _exit(127);
        }

        // Parent
        pids[idx] = pid;
        free(argv);
    }

    // Parent closes all pipe ends
    for (int k = 0; k < pipes_needed; k++) {
        close(pipes[k][0]);
        close(pipes[k][1]);
    }

    if (bg) {
        // Background: don't wait; print the first PID (like a job id hint)
        printf("[background] started pipeline (pids:");
        for (int idx = 0; idx < n; idx++) printf(" %d", (int)pids[idx]);
        printf(" )\n");
        fflush(stdout);
    } else {
        // Foreground: wait for all
        int status;
        for (int idx = 0; idx < n; idx++) {
            while (waitpid(pids[idx], &status, 0) < 0 && errno == EINTR) { /* retry */ }
        }
    }
}

int main(void) {
    char line[MAXLINE];
    char *tokens[MAX_NUM_TOKENS];
    Command commands[MAX_NUM_COMMANDS];

    while (1) {
        printf("%% ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        // Strip trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (strcmp(line, "exit") == 0) break;

        int ntok = tokenise(line, tokens);
        if (ntok == -1) {
            printf("Error: too many tokens!\n");
            continue;
        }
        if (ntok == 0) continue;

        int ncmd = separateCommands(tokens, ntok, commands);
        if (ncmd <= 0) continue;

        // Group commands by pipelines; decide wait/background by last separator in group
        for (int i = 0; i < ncmd; ) {
            int j = i;
            // Extend through pipes
            while (j < ncmd - 1 && is_pipe_sep(commands[j].separator)) j++;

            // Decide background/foreground by separator after the last command in the group
            int bg = 0;
            if (is_bg_sep(commands[j].separator)) bg = 1;
            // ';' or NULL -> wait (bg=0)

            run_pipeline(tokens, commands, i, j, bg);
            i = j + 1;
        }
    }

    return 0;
}
