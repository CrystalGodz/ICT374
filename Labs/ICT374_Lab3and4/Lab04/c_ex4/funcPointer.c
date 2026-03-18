#include <stdio.h>

// A function that squares its argument
int square(int n) {
    return n * n;
}

int main(void) {
    int (*fp)(int);   // declare a function pointer
    int n;

    // Print the address of the function
    printf("Address of function square: %p\n", (void *)square);

    // Assign function address to function pointer
    fp = square;

    // Call function directly
    n = square(10);
    printf("square(10) = %d (direct call)\n", n);

    // Call function via pointer
    n = fp(10);
    printf("fp(10) = %d (via function pointer)\n", n);

    return 0;
}
