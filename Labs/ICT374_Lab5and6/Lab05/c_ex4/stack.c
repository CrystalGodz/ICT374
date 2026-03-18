/* file:     stack.c
 * aim:      implement a stack
 * author:   HX
 * date:     2006.09.06
 */

#include <stdlib.h>
#include <stdio.h>
#include "stack.h"

Stack *create_stack()
{
     Stack *s;

     s = malloc(sizeof(Stack));

     if (s==NULL) {
         fprintf(stderr, "No more memory - quit\n");
         exit(1);
     }

     s->top = NULL;
     s->number = 0;
     return s;
}

void push_stack(Stack *stack, char *str)
{
     Node *newNode;

     newNode = malloc(sizeof(Node));
     if (newNode == NULL) {
          fprintf(stderr, "No more memory - quit!\n");
          exit(1);
     }

     newNode -> str = str;
     newNode->next = stack->top;
     stack->top = newNode;
     stack -> number += 1;
}

char *pop_stack(Stack *stack)
{
     Node *np;
     char *str;

     if (empty_stack(stack))
         return NULL;

     np = stack->top;
     stack->top = np->next;
     stack->number -= 1;

     str = np->str;
     free(np);
     return str;
}

int size_of_stack(Stack *stack)
{
    return stack->number;
}

int empty_stack(Stack *stack)
{
    return stack->number == 0;
}
