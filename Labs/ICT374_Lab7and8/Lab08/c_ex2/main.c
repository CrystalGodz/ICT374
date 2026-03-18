#include <stdio.h>
#include <string.h>
#include "token.h"
#include "command.h"

#define MAXLINE 1000
#define MAX_NUM_COMMANDS 100

int main(void) {
    char line[MAXLINE];
    char *tokens[MAX_NUM_TOKENS];
    Command commands[MAX_NUM_COMMANDS];

    while (1) {
        printf("%% ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (strcmp(line, "exit") == 0) break;

        int ntok = tokenise(line, tokens);
        if (ntok <= 0) continue;

        int ncmd = separateCommands(tokens, ntok, commands);

        for (int i = 0; i < ncmd; i++) {
            printf("Command %d:\n", i);
            for (int j = 0; commands[i].argv[j] != NULL; j++) {
                printf("  argv[%d] = %s\n", j, commands[i].argv[j]);
            }
            if (commands[i].stdin_file)
                printf("  stdin redirect: %s\n", commands[i].stdin_file);
            if (commands[i].stdout_file)
                printf("  stdout redirect: %s\n", commands[i].stdout_file);
            printf("  separator: %s\n",
                   commands[i].separator ? commands[i].separator : "(none)");
        }
    }

    return 0;
}
