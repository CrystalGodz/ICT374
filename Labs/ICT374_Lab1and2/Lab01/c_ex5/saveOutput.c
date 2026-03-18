#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXLINE 1000  // maximum input line size

int main(void) {
    char line[MAXLINE];
    int n, i;
    char *fname = "foo2";
    FILE *fstr;

    // Open file for writing (creates foo2 if it doesn’t exist, overwrites if it does)
    printf("Enter a word or sentence: ");
    fstr = fopen(fname, "w+");
    if (fstr == NULL) {
        fprintf(stderr, "Cannot open file %s, terminate program\n", fname);
        exit(1);
    }

    // Read a line from standard input
    if (fgets(line, sizeof(line), stdin) == NULL) {
        fclose(fstr);
        return 1; // no input
    }

    // Remove the newline if present
    n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') {
        line[n - 1] = '\0';
        n--; // adjust length
    }

    // Write the reversed line to foo2
    for (i = n - 1; i >= 0; --i) {
        fputc(line[i], fstr);  // write one char at a time
    }
    fputc('\n', fstr); // add a newline

    // Close the file
    fclose(fstr);

    return 0;
}
