/*
* cli6.c An improved version of "cli5.c". Since TCP does
* not preserve the message boundaries, each message
* is preceded by a two byte value which is the length
* of the message.
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>   // struct sockaddr_in, htons, htonl
#include <netdb.h>        // struct hostent, gethostbyname()
#include <string.h>
#include "stream.h"       // MAX_BLOCK_SIZE, readn(), writen()

#define SERV_TCP_PORT 40003 // server's "well-known" port number

int main(int argc, char *argv[])
{
    int sd, n, nr, nw, i=0;
    char buf[MAX_BLOCK_SIZE], host[60];
    struct sockaddr_in ser_addr;
    struct hostent *hp;

    // get server host name
    if (argc == 1)                // assume server running on the local host
        gethostname(host, sizeof(host));
    else if (argc == 2)           // use the given host name
        strcpy(host, argv[1]);
    else {
        printf("Usage: %s [<server_host_name>]\n", argv[0]);
        exit(1);
    }

    // get host address, & build a server socket address
    bzero((char *) &ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(SERV_TCP_PORT);
    if ((hp = gethostbyname(host)) == NULL) {
        printf("host %s not found\n", host);
        exit(1);
    }
    ser_addr.sin_addr.s_addr = *(u_long *) hp->h_addr;

    // create TCP socket & connect socket to server address
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0) {
        perror("client connect");
        exit(1);
    }

    while (++i) {
        printf("Client Input[%d]: ", i);
        fgets(buf, sizeof(buf), stdin);
        nr = strlen(buf);
        if (buf[nr-1] == '\n') { buf[nr-1] = '\0'; --nr; }

        if (strcmp(buf, "quit") == 0) {
            printf("Bye from client\n");
            exit(0);
        }

        if (nr > 0) {
            if ((nw = writen(sd, buf, nr)) < nr) {
                printf("client: send error\n");
                exit(1);
            }
            if ((nr = readn(sd, buf, sizeof(buf))) <= 0) {
                printf("client: receive error\n");
                exit(1);
            }
            buf[nr] = '\0';
            printf("Server Output[%d]: %s\n", i, buf);
        }
    }
}
