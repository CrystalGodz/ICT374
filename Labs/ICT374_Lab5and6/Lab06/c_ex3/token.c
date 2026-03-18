#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "token.h"

int tokenise(char *inputLine, char *token[]) {
    int count = 0;
    char *p = inputLine;

    while (*p != '\0') {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n') {
            p++;
        }

        if (*p == '\0') {
            break; // end of string
        }

        // Found start of a token
        if (count >= MAX_NUM_TOKENS) {
            return -1; // too many tokens
        }
        token[count++] = p;

        // Move until end of token
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n') {
            p++;
        }

        // Null-terminate token
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    return count;
}
