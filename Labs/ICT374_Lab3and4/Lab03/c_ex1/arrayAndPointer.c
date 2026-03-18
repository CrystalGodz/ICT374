#include <stdio.h>

int main(void) {
    int ai[10];
    int *pi;
    int i, n;

    // Initialise array elements
    for (i = 0; i < 10; ++i) {
        ai[i] = i + 100;
    }

    // Make pi point to start of ai
    pi = ai;   // same as pi = &ai[0];

    printf("Access array elements using six different notations:\n\n");

    // 1) n = ai[i];
    printf("1) Using ai[i]\n");
    for (i = 0; i < 10; ++i) {
        n = ai[i];
        printf("ai[%d] = %d\n", i, n);
    }
    printf("\n");

    // 2) n = pi[i];
    printf("2) Using pi[i]\n");
    for (i = 0; i < 10; ++i) {
        n = pi[i];
        printf("pi[%d] = %d\n", i, n);
    }
    printf("\n");

    // 3) n = *(ai + i);
    printf("3) Using *(ai + i)\n");
    for (i = 0; i < 10; ++i) {
        n = *(ai + i);
        printf("*(ai + %d) = %d\n", i, n);
    }
    printf("\n");

    // 4) n = *(pi + i);
    printf("4) Using *(pi + i)\n");
    for (i = 0; i < 10; ++i) {
        n = *(pi + i);
        printf("*(pi + %d) = %d\n", i, n);
    }
    printf("\n");

    // 5) pi = pi + i; n = *pi;
    printf("5) Using pi = pi + i; n = *pi;\n");
    pi = ai; // reset pointer to start of array
    for (i = 0; i < 10; ++i) {
        int *tmp = pi + i;  // avoid moving pi itself permanently
        n = *tmp;
        printf("*(pi + %d) = %d\n", i, n);
    }
    printf("\n");

    // 6) pi = ai + i; n = *pi;
    printf("6) Using pi = ai + i; n = *pi;\n");
    for (i = 0; i < 10; ++i) {
        pi = ai + i;
        n = *pi;
        printf("*(ai + %d) = %d\n", i, n);
    }
    printf("\n");

    return 0;
}
