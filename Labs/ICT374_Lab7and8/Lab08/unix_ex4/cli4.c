/*
* cli4.c a client for reversing strings, using TCP socket.
* The server machine address (134.115.64.72) is hardcoded.
*/
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in, htons, htonl
#include <string.h>
#include <stdio.h>
#define BUFSIZE 256
#define SERV_TCP_PORT 40000 // "well-known" server port number
#define SERV_INET_NO 3232272768 // "192.168.145.128"

int main()
{
    int sd, n, nr, nw, i=0;
    char buf[BUFSIZE];
    struct sockaddr_in ser_addr;

    // build a server socket address
    bzero((char *) &ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(SERV_TCP_PORT);
    ser_addr.sin_addr.s_addr = htonl(SERV_INET_NO);

    // create TCP socket & connect socket to server address
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) {
        perror("client connect"); exit(1);
    }

    while (++i) {
        printf("Client Input[%d]: ", i);
        fgets(buf, BUFSIZE, stdin); // get a message from user
        nr = strlen(buf);
        if (buf[nr-1] == '\n') { buf[nr-1] = '\0'; --nr; }
        if (strcmp(buf, "quit")==0) { // is the message "quit"?
            printf("Bye from client\n");
            exit(0);
        }
        if (nr > 0) {
            nw = write(sd, buf, nr);
            nr = read(sd, buf, BUFSIZE); buf[nr] = '\0';
            printf("Sever Output[%d]: %s\n", i, buf);
        }
    }
}
