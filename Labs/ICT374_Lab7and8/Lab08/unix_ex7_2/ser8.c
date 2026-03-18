/*
* ser8.c An UDP version of reverse string service (server part).
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <strings.h>
#include <errno.h> // extern int errno, EINTR, perror()
#include <sys/types.h> // pid_t, u_long, u_short
#include <sys/socket.h> // struct sockaddr, socket(), etc
#include <netinet/in.h> // struct sockaddr_in, htons(), htonl()
 // and INADDR_ANY
#define BUFSIZE 256
#define SERV_UDP_PORT 40001 // server port no
void daemon_init(void)
{
 pid_t pid;
 if ( (pid = fork()) < 0) {
 perror("fork"); exit(1);
 } else if (pid > 0)
 exit(0); // parent goes bye-bye
 // child continues
 setsid(); // become session leader
 chdir("/"); // change working directory
 umask(0); // clear file mode creation mask
}
void reverse(char *s, int len)
{
 char c;
 int i, j;
 for (i=0, j = len-1; i<j; i++, j--) {
 c = s[i]; s[i] = s[j]; s[j] = c;
 }
}
int main()
{
 int sd, cli_addr_len, n; char buf[BUFSIZE];
 struct sockaddr_in ser_addr, cli_addr;

 // turn the program into a daemon
 daemon_init();
 // set up server socket sd

 if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
 perror("server:socket"); exit(1);
 }
 // build server Internet socket address
 bzero((char *)&ser_addr, sizeof(ser_addr));
 ser_addr.sin_family = AF_INET;
 ser_addr.sin_port = htons(SERV_UDP_PORT);
 ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 /* note: accept client request sent to any one of the
 network interface(s) on this host.
 */
 // bind server address to socket sd
 if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0){
 perror("server bind"); exit(1);
 }
 while (1) {
 // wait for a client request
 cli_addr_len = sizeof(cli_addr);
 n = recvfrom(sd, buf, sizeof(buf), 0,
 (struct sockaddr *)&cli_addr,
 (socklen_t *)&cli_addr_len);
 if (n <= 0) exit(1); // receive error
 // process the client message
 reverse(buf+4, n-4); // do not change the message id */

 // send the reversed message to that client
 n = sendto(sd, buf, n, 0, (struct sockaddr *) &cli_addr,
 (socklen_t) cli_addr_len);
 if (n <= 0) exit(1); // send error
 }
}
