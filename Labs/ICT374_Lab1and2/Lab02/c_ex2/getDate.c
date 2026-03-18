#include <stdio.h>

#define BUF_SIZE 10  // enough to hold 3-letter month + '\0'

int main(void) {
    int date, year;
    char month[BUF_SIZE];

    printf("Please enter the date in the form of date-short_month-year,\n");
    printf("such as 8-Aug-2006, where the month is written in exactly three letters:\n");

    if (scanf("%d-%3s-%d", &date, month, &year) != 3) {
        printf("Invalid input format!\n");
        return 1;
    }

    printf("Day: %d\n", date);
    printf("Month: %s\n", month);
    printf("Year: %d\n", year);

    return 0;
}
