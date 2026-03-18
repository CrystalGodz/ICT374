#include <stdio.h>
#include <string.h>
#include "token.h"

#define MAXLINE 1000

int main(void) {
    char line[MAXLINE];
    char *tokens[MAX_NUM_TOKENS];
    int ntok, i;

    while (1) {
        // Display a prompt
        printf("%% ");
        fflush(stdout);

        // Read a line
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break; // EOF
        }

        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Exit command
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Tokenise
        ntok = tokenise(line, tokens);
        if (ntok == -1) {
            printf("Error: too many tokens!\n");
            continue;
        }

        // Print tokens
        printf("Found %d tokens:\n", ntok);
        for (i = 0; i < ntok; i++) {
            printf("Token[%d] = \"%s\"\n", i, tokens[i]);
        }
    }

    printf("Goodbye!\n");
    return 0;
}
