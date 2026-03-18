#include <string.h>
#include "command.h"

static int is_sep(const char *t) {
    return (strcmp(t, "|") == 0) || (strcmp(t, "&") == 0) || (strcmp(t, ";") == 0);
}

int separateCommands(char *tokens[], int ntok, Command command[]) {
    int ncmd = 0;
    int start = 0;

    for (int i = 0; i < ntok; i++) {
        if (is_sep(tokens[i])) {
            // Command is from [start .. i-1]
            if (i > start) {
                command[ncmd].first = start;
                command[ncmd].last = i - 1;
                command[ncmd].separator = tokens[i];
                ncmd++;
            } else {
                // Two separators in a row; ignore empty command
            }
            start = i + 1;
        }
    }

    // Trailing command (no separator after it)
    if (start < ntok) {
        command[ncmd].first = start;
        command[ncmd].last = ntok - 1;
        command[ncmd].separator = NULL;
        ncmd++;
    }

    return ncmd;
}
