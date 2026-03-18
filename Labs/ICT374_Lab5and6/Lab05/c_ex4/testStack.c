/* file:      test_stack.c
 * aim:       to test stack module
 * author:    HX
 * date:      2006.09.06
 */

#include <stdlib.h>
#include <stdio.h>
#include "stack.h"

int main(int argc, char *argv[])
{
     Stack *s;
     int i;

     // create an empty stack
     s = create_stack();

     // push all command line arguments into the stack
     for (i=0; i<argc; ++i)
        push_stack(s, argv[i]);

     printf("there are %d strings in the stack\n", size_of_stack(s));

     // pop out the strings in the stack and print them out
     while ( !empty_stack(s) )
          printf("Stack item: %s\n", pop_stack(s));

     printf("now there are %d strings in the stack\n", size_of_stack(s));

    exit(0);
}
