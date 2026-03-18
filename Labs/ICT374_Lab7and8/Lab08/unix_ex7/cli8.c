/*
* cli8.c An UDP version of string reverse (client part).
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h> // struct hostent, gethostbyname()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in, htons, htonl
#define BUFSIZE 256
#define SERV_UDP_PORT 40001 // server's "well-known" port number
#define MAX_RETRY 10
#define INIT_INTERVAL 1
int alarm_expired = 0;
void alarm_expire()
{
 alarm_expired = 1;
}
int main(int argc, char *argv[])
{
 unsigned short message_no, retry_no, retry, i=0;
 int sd, n, nr, nw, interval, ser_addr_len;
 char buf1[BUFSIZE], buf2[BUFSIZE], host[100];
 char *mesg = buf1+4;
 struct sockaddr_in ser_addr; struct hostent *hp;
 // get server host name
 if (argc==1) // assume server running on the local host
 gethostname(host, sizeof(host));
 else if (argc == 2) // use the given host name
 strcpy(host, argv[1]);
 else {
 printf("Usage: %s [<server_host_name>]\n", argv[0]); exit(1);
 }
 // get host address, & build a server socket address
 bzero((char *) &ser_addr, sizeof(ser_addr));
 ser_addr.sin_family = AF_INET;
 ser_addr.sin_port = htons(SERV_UDP_PORT);
 if ((hp = gethostbyname(host)) == NULL){
 printf("host %s not found\n", host); exit(1);
 }
 ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;
 ser_addr_len = sizeof(ser_addr);
 // create UDP socket
 sd = socket(AF_INET, SOCK_DGRAM, 0);

 // set alarm clock handler
 signal(SIGALRM, alarm_expire);

 while (++i) {
 // get a message from user, place it in buf1+4
 printf("\nClient Input[%d] :", i);
 fgets(mesg, BUFSIZE-5, stdin); nr = strlen(mesg);
 if (mesg[nr-1] == '\n') { mesg[nr-1] = '\0'; --nr; }
 if (nr <= 0) continue;
 if (strcmp(mesg, "quit")==0) {
 printf("Bye from client\n"); exit(0);
 }

 interval = INIT_INTERVAL; // initial waiting time
 message_no = htons(i); // add the header
 bcopy((char *) &message_no, buf1, 2);
 n = nr+4; // total message length, including the header
 // try to send the message i in buf1 and get its reply
 for (retry=0; retry < MAX_RETRY; retry++) {
 if (retry) printf("Client Retry No. %d ...\n", retry);
 // add retry no to header
 retry_no = htons(retry);
 bcopy((char *)&retry_no, buf1+2, 2);
 // send the message id and message body to server
 nw = sendto(sd, buf1, n, 0,
 (struct sockaddr *) &ser_addr, ser_addr_len);
 alarm (interval); // set alarm clock
 nr = recv(sd, buf2, BUFSIZE, 0);
 if (nr <=0 ) {
 if (alarm_expired) {
 alarm_expired = 0; interval *= 2;
 } else {
 perror("recv"); close(sd); exit(1);
 }
 } else { // a reply is received
 buf2[nr] = '\0'; alarm(0);
message_no = ntohs( * (short *) buf2 );
retry_no = ntohs( * (short *) (buf2+2) );
if (message_no == i) break; // got its reply
 printf("Client: discard message[%d]: %s\n",
 message_no, buf2+4);
 }
 }
 if (retry < MAX_RETRY)
 printf("Sever Output[%d] (retry_no=%d): %s\n",
 i, retry_no, buf2+4);
 else {
 printf("client: message[%d] lost\n", i);
 close(sd); exit(1);
 }
 }
}
