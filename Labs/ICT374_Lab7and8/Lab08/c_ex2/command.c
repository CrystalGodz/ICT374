#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"

void searchRedirection(char *tokens[], Command *cp) {
    cp->stdin_file = NULL;
    cp->stdout_file = NULL;

    for (int i = cp->first; i <= cp->last; i++) {
        if (strcmp(tokens[i], "<") == 0 && i + 1 <= cp->last) {
            cp->stdin_file = tokens[i + 1];
            i++;
        } else if (strcmp(tokens[i], ">") == 0 && i + 1 <= cp->last) {
            cp->stdout_file = tokens[i + 1];
            i++;
        }
    }
}

void buildCommandArgumentArray(char *tokens[], Command *cp) {
    int n = (cp->last - cp->first + 1);

    if (cp->stdin_file) n -= 2;
    if (cp->stdout_file) n -= 2;

    n += 1; // for NULL terminator

    cp->argv = realloc(cp->argv, sizeof(char *) * n);
    if (cp->argv == NULL) {
        perror("realloc");
        exit(1);
    }

    int k = 0;
    for (int i = cp->first; i <= cp->last; i++) {
        if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0) {
            i++; // skip redirection symbol + filename
        } else {
            cp->argv[k++] = tokens[i];
        }
    }
    cp->argv[k] = NULL;
}

int separateCommands(char *tokens[], int ntok, Command command[]) {
    int ncmd = 0;
    int start = 0;

    for (int i = 0; i < ntok; i++) {
        if (strcmp(tokens[i], "|") == 0 ||
            strcmp(tokens[i], "&") == 0 ||
            strcmp(tokens[i], ";") == 0) {

            command[ncmd].first = start;
            command[ncmd].last = i - 1;
            command[ncmd].separator = tokens[i];
            command[ncmd].argv = NULL;
            command[ncmd].stdin_file = NULL;
            command[ncmd].stdout_file = NULL;
            ncmd++;
            start = i + 1;
        }
    }

    if (start < ntok) {
        command[ncmd].first = start;
        command[ncmd].last = ntok - 1;
        command[ncmd].separator = NULL;
        command[ncmd].argv = NULL;
        command[ncmd].stdin_file = NULL;
        command[ncmd].stdout_file = NULL;
        ncmd++;
    }

    // process redirection + build argv
    for (int i = 0; i < ncmd; i++) {
        searchRedirection(tokens, &command[i]);
        buildCommandArgumentArray(tokens, &command[i]);
    }

    return ncmd;
}

