#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int i;

    if (argc == 1) {
        printf("Usage: %s [options] [files...]\n", argv[0]);
        exit(0);
    }

    printf("There are %d command line arguments (including program name).\n", argc);

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            // It's an option (starts with '-')
            printf("Option: %s\n", argv[i]);
        }
        else {
            // It's not an option → treat as filename or operand
            printf("File/Argument: %s\n", argv[i]);
        }
    }

    return 0;
}
