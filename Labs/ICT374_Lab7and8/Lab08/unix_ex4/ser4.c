/*
* ser4.c a server for reversing strings, using TCP stream socket.
* Server machine's address (192.168.145.128) is hard
* coded in the server & client.
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in, htons, htonl
#include <string.h>
#define BUFSIZE 256
#define SERV_INET_NO 3232272768 // IP number 192.168.145.128
#define SERV_TCP_PORT 40000     // server port number

void daemon_init(void)
{
    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("fork"); exit(1);
    } else if (pid > 0)
        exit(0); // parent goes bye-bye
    // child continues
    setsid();    // become session leader
    chdir("/");  // change working directory
    umask(0);    // clear our file mode creation mask
}

void reverse(char *s)
{
    char c; int i, j;
    for (i=0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i]; s[i] = s[j]; s[j] = c;
    }
}

void serve_a_client(int sd)
{
    int nr, nw;
    char buf[BUFSIZE];

    while (1) {
        // read a message from new socket sd
        if ((nr = read(sd, buf, sizeof(buf)-1)) <= 0)
            exit(0); // connection down

        buf[nr] = '\0'; // null terminate string

        // process the message
        reverse(buf);

        // send result back
        if ((nw = write(sd, buf, strlen(buf))) < 0) {
            perror("write");
            exit(1);
        }
    }
}

int main()
{
    int listenfd, connfd, clilen;
    struct sockaddr_in cliaddr, servaddr;

    daemon_init(); // run as a daemon

    // create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // initialize server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(SERV_INET_NO);
    servaddr.sin_port = htons(SERV_TCP_PORT);

    // bind socket
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // listen
    listen(listenfd, 5);

    for (;;) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);

        if (fork() == 0) { // child
            close(listenfd);
            serve_a_client(connfd);
            exit(0);
        }
        close(connfd); // parent closes
    }
}
