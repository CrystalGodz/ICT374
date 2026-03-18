#include <stdio.h>
#include <stdlib.h>

// Function that creates, fills, and returns a dynamic array
int *getArray(int arraySize) {
    int *array;

    // Allocate memory for arraySize integers
    array = (int *)malloc(arraySize * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    // Fill the array with values (user input here)
    for (int i = 0; i < arraySize; i++) {
        printf("Enter element %d: ", i);
        scanf("%d", &array[i]);
    }

    return array; // return pointer to caller
}

// Helper function to print an array
void printArray(int *array, int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

int main(void) {
    int *pi;
    int n;

    printf("Enter array size: ");
    scanf("%d", &n);

    // Create and fill dynamic array
    pi = getArray(n);

    // Print the array
    printf("You entered: ");
    printArray(pi, n);

    // Free the memory when done
    free(pi);

    return 0;
}
