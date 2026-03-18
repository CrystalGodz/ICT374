#ifndef COMMAND_H
#define COMMAND_H

#include "token.h"

struct CommandStructure {
    int first;          // index of first token
    int last;           // index of last token
    char *separator;    // "|" or "&" or ";" or NULL

    // new fields
    char **argv;        // NULL-terminated array of arguments
    char *stdin_file;   // input redirection file
    char *stdout_file;  // output redirection file
};

typedef struct CommandStructure Command;

// split tokens into commands
int separateCommands(char *tokens[], int ntok, Command command[]);

// redirection scanner
void searchRedirection(char *tokens[], Command *cp);

// build argv[] array for execvp
void buildCommandArgumentArray(char *tokens[], Command *cp);

#endif
