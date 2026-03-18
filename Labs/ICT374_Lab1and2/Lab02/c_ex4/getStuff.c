#include <stdio.h>

#define BUF_SIZE 10  // enough for 3-char month + null terminator

int main(void) {
    const char *date_str1 = "8-Aug-2006";
    const char *date_str2 = "25-Dec-1999";
    const char *date_str3 = "1-Jan-2024";

    int date, year;
    char month[BUF_SIZE];

    // Test 1
    if (sscanf(date_str1, "%d-%3s-%d", &date, month, &year) == 3) {
        printf("Date string: %s\n", date_str1);
        printf("Parsed: %s %d, %d\n\n", month, date, year);
    }

    // Test 2
    if (sscanf(date_str2, "%d-%3s-%d", &date, month, &year) == 3) {
        printf("Date string: %s\n", date_str2);
        printf("Parsed: %s %d, %d\n\n", month, date, year);
    }

    // Test 3
    if (sscanf(date_str3, "%d-%3s-%d", &date, month, &year) == 3) {
        printf("Date string: %s\n", date_str3);
        printf("Parsed: %s %d, %d\n\n", month, date, year);
    }

    return 0;
}
