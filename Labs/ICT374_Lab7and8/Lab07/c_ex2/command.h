#ifndef COMMAND_H
#define COMMAND_H

#include "token.h"

#define MAX_NUM_COMMANDS 100

struct CommandStructure {
    int first;          // index of the first token of the command
    int last;           // index of the last token of the command
    char *separator;    // "|" or "&" or ";" if one follows, else NULL
};

typedef struct CommandStructure Command;

// Break a list of tokens into commands
// Returns number of commands found
int separateCommands(char *tokens[], int ntok, Command command[]);

#endif
