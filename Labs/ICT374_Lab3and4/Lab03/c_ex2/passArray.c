#include <stdio.h>

// Function prototype
void fillArray(int array[], int arraySize);
void printArray(int array[], int arraySize);

int main(void) {
    int arr1[5];
    int arr2[3];

    printf("Fill arr1 (size 5):\n");
    fillArray(arr1, 5);
    printf("You entered arr1: ");
    printArray(arr1, 5);

    printf("\nFill arr2 (size 3):\n");
    fillArray(arr2, 3);
    printf("You entered arr2: ");
    printArray(arr2, 3);

    return 0;
}

// Fill array with user input
void fillArray(int array[], int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        printf("Enter element %d: ", i);
        scanf("%d", &array[i]);
    }
}

// Print array contents
void printArray(int array[], int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}
