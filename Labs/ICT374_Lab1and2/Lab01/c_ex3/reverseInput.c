#include <stdio.h>
#include <string.h>

#define MAXLINE 1000  // maximum input line size

int main(void) {
    char line[MAXLINE];
    int n, i;

    // Read a line from standard input
    printf("Enter a word or sentence: ");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return 1; // no input
    }

    // Remove the newline if present
    n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') {
        line[n - 1] = '\0';
        n--; // adjust length
    }

    // Print the reversed line
    for (i = n - 1; i >= 0; --i) {
        putchar(line[i]);
    }
    putchar('\n');

    return 0;
}
