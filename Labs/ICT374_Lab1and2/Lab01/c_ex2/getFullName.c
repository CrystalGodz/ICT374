/*  file:	getinput.c
 *  author:	HX
 *  date:	01/08/06
 *  revised:	08/09/07
 */

#include <stdio.h>	// for printf and fgets
#include <stdlib.h>	// for exit

#define BUFSIZE   128

int main(void)
{
     char name[BUFSIZE];

     printf("Type your full name: ");
     fgets(name, BUFSIZE, stdin);
     
     printf("Your name is %s\n", name);
     
     exit(0);
} 
