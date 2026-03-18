/*
* stream.h head file for stream read and stream write.
*/
#define MAX_BLOCK_SIZE (1024*5) // maximum size of any piece of data that can be sent by client

int readn(int fd, char *buf, int bufsize);
/*
* return value:
*  > 0 : number of bytes read
*  = 0 : connection closed
*  = -1 : read error
*  = -2 : protocol error
*  = -3 : buffer too small
*/

int writen(int fd, char *buf, int nbytes);
/*
* return value:
*  = nbytes : number of bytes written
*  = -3 : too many bytes to send
*  otherwise: write error
*/
