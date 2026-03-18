/* name:          memory.c
 * aims:          to see how the os and the compiler allocates memory for different regions of
 *                a process (user visible part), including text region, data region, 
 *                heap, stack, command line arguments and process environment region
 *
 * author:        HX
 * updated: 	  2025.09.09
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

extern char **environ;


int gx = 10;   // initialised global
int gy;  // uninitialised global
char gname1[] = "Hi, there!";
char *gname2  = "operating system";  
const int gc = 100;
int gz;

void printAddress(char *description, void *addr)
{
    unsigned long a = (unsigned long) addr;

    unsigned long b  = a & 0x3ff;
    unsigned long kib = a >> 10; kib = kib & 0x3ff;
    unsigned long mib = a >> 20; mib = mib & 0x3ff;
    unsigned long gib = a >> 30; gib = gib & 0x3ff;
    unsigned long tib = a >> 40; tib = tib & 0x3ff;
    printf("%s, %16p,  %lu,  %lu, %lu, %lu, %lu, %lu\n", description, addr, a, tib, gib, mib, kib, b);

    return;
}

int f1(int x1, int x2, float x3, double x4, char x5, int x6 )
{
    int     f1_l1;
    float   f1_l2;
    char    f1_l3;
    char    f1_l3b;
    double  f1_l4;
    const int  f1_l5 = 123;
    int     f1_l6;

    // print the addresses of all formal parameters of function f1
    printAddress("formal parameter, f1, &x1", &x1);
    printAddress("formal parameter, f1, &x2", &x2);
    printAddress("formal parameter, f1, &x3", &x3);
    printAddress("formal parameter, f1, &x4", &x4);
    printAddress("formal parameter, f1, &x5", &x5);
    printAddress("formal parameter, f1, &x6", &x6);

    // print the addresses of all local variables of function f1
    printAddress("local variable, f1, &f1_l1", &f1_l1);
    printAddress("local variable, f1, &f1_l2", &f1_l2);
    printAddress("local variable, f1, &f1_l3", &f1_l3);
    printAddress("local variable, f1, &f1_l3b", &f1_l3b);
    printAddress("local variable, f1, &f1_l4", &f1_l4);
    printAddress("local variable, f1, &f1_l5", (void *)&f1_l5);
    printAddress("local variable, f1, &f1_l6", &f1_l6);

    return 0;
}

void f2()
{
    #define BUFSIZE 1024*1024
    char f2_buf[BUFSIZE];
    char *f2_p1;
    char *f2_p2;
    f2_p1 = malloc(100);
    if (f2_p1==NULL) {
         perror("mallc memory");
         exit(1);
    }
    f2_p2 = malloc(BUFSIZE);
    if (f2_p2==NULL) {
         perror("mallc memory");
         exit(1);
    }

    // print the addresses of all local variables in f2
    printAddress("local variable, f2, &f2_buf", &f2_buf);
    printAddress("local variable, f2, &f2_p1", &f2_p1);
    printAddress("local variable, f2, &f2_p2", &f2_p2);

    // print the addresses of all dynamically allocated variables in f2
    printAddress("dynamic variable, f2, f2_p1", f2_p1);
    printAddress("dynamic variable, f2, f2_p2", f2_p2);

    //==== call function f1 in function f2  ====
    f1(10, 20, 10.2, 20.3, 'a', 100);

    return;
}

int main(int argc, char *argv[], char *env[])
{
    char main_buf[256];

    //==== print excel spreadsheet header =====
    printf("Type, Scope, Syntax, Hexadecimal, Decimal, TiB,GiB,MiB,KiB,B\n");

    // end of text, end of init data and end of data,  linux only
    extern char etext; // end of text region
    extern char edata; // end of the initialised data
    extern char end;   // end of uninitialised data
    printAddress("end of text, global, &etext", &etext);
    printAddress("end of initialised data, global, &edata", &edata);
    printAddress("end of data region, global, &end", &end);

    //==== program text ====
    // print the addresses of all functions defined in the program
    printAddress("function, global, f1", f1);
    printAddress("function, global, f2", f2);
    printAddress("function, global, main", main);

    //==== constants and literals ====
    // print the addresses of global constants and string literal "operating system"
    printAddress("constant, global, &gc", (void *)&gc);
    printAddress("string literal, global, gname2", gname2);

    //==== initialised globals ====
    // print the addresses of  initialised global variables 
    printAddress("initialised global, &gx", &gx);
    printAddress("initialised global, gname1", gname1);

    //==== uninitialised globals ====
    // print the addresses of uninitialised global variables
    printAddress("uninitialised global, &gy", &gy);
    printAddress("uninitialised global, &gz", &gz);

    //==== extern variables ====
    // print the addresses of extern variable 
    printAddress("extern variable, environ", environ);

    //==== formal parameters in function main ====
    // print the addresses of formal parameters argc, argv, and env
    printAddress("formal parameter, main, &argc", &argc);
    printAddress("formal parameter, main, argv", argv);
    printAddress("formal parameter, main, env", env);

    //==== allocate memory from heap ====
    char *main_p1 = malloc(40000*1024);
    char *main_p2 = malloc(10000);

    //==== local variables in main ====
    // print the addresses of local variables main_buf, main_p1, main_p2
    printAddress("local variable, main, &main_buf", &main_buf);
    printAddress("local variable, main, &main_p1", &main_p1);
    printAddress("local variable, main, &main_p2", &main_p2);

    //==== dynamic variables ====
    // print the addresses of dynamically allocated variables pointed to by main_p1 and main_p2
    printAddress("dynamic variable, main, main_p1", main_p1);
    printAddress("dynamic variable, main, main_p2", main_p2);

    //==== call function f2 from main function ====
    f2();

    //==== command line arguments ====
    // print the start address of the array of pointers point to cmd line arguments 
    printAddress("argv array start, main, argv", argv);

    // print the end addresses of the array of pointers point to cmd line arguments 
    printAddress("argv array end, main, argv+argc", argv+argc);

    // print start addresses of each command line argument
    for (int i = 0; i < argc; i++) {
        char buf[128];
        sprintf(buf, "argv[%d]", i);
        printAddress(buf, argv[i]);
    }

    // print the end address of the last command line argument
    printAddress("argv last end", argv[argc-1] + strlen(argv[argc-1]));

    //==== environment ====
    // find out the number of environment variables
    int nv = 0;   // number of environment variables
    while (env[nv]) ++nv;

    // print the start addresses of the array of pointers to the list of env variables 
    printAddress("env array start", env);

    // print the end addresses of the array of pointers to the list of env variables 
    printAddress("env array end", env+nv);

    // print the start address of the 0th environment variable
    printAddress("env[0]", env[0]);

    // print the start address of the last environment variable
    printAddress("env[last]", env[nv-1]);

    // print the end address of the last environment variables
    sprintf(main_buf, "end of the last env variable, , env[%d]+strlen(env[%d])", nv-1, nv-1);
    printAddress(main_buf, env[nv-1] + strlen(env[nv-1]));

    //=== program break ====
    // note: the heap region is between the end of data region and the program break 
    printAddress("program break, , sbrk(0)", sbrk(0));

    // type a key to end the program 
    char c = getchar();

    exit(0);
}
