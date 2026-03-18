/*
* ser6.c An improved TCP string-reversal server using message framing.
*/
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include "stream.h"

#define SERV_TCP_PORT 40003 // server port no

void claim_children()
{
    pid_t pid=1;
    while (pid>0) {
        pid = waitpid(0, (int *)0, WNOHANG);
    }
}

void daemon_init(void)
{
    pid_t pid;
    struct sigaction act;
    if ((pid = fork()) < 0) {
        perror("fork"); exit(1);
    } else if (pid > 0)
        exit(0);
    setsid();
    chdir("/");
    umask(0);
    act.sa_handler = claim_children;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &act, 0);
}

void reverse(char *s)
{
    char c;
    int i, j;
    for (i=0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i]; s[i] = s[j]; s[j] = c;
    }
}

void serve_a_client(int sd)
{
    int nr, nw;
    char buf[MAX_BLOCK_SIZE];

    while (1) {
        if ((nr = readn(sd, buf, sizeof(buf))) <= 0)
            return;
        buf[nr] = '\0';
        reverse(buf);
        nw = writen(sd, buf, nr);
    }
}

int main()
{
    int sd, nsd, cli_addrlen; pid_t pid;
    struct sockaddr_in ser_addr, cli_addr;

    daemon_init();

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server:socket"); exit(1);
    }

    bzero((char *)&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(SERV_TCP_PORT);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr))<0) {
        perror("server bind"); exit(1);
    }

    listen(sd, 5);

    while (1) {
        cli_addrlen = sizeof(cli_addr);
        nsd = accept(sd, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
        if (nsd < 0) {
            if (errno == EINTR)
                continue;
            perror("server:accept"); exit(1);
        }

        if ((pid=fork()) <0) {
            perror("fork"); exit(1);
        } else if (pid > 0) {
            close(nsd);
            continue;
        }

        close(sd);
        serve_a_client(nsd);
        exit(0);
    }
}
