/*
* stream.c
* routines for stream read and write.
*/
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in, htons(), htonl()
#include "stream.h"

int readn(int fd, char *buf, int bufsize)
{
    short data_size;
    int n, nr, len;

    if (bufsize < MAX_BLOCK_SIZE)
        return -3;

    if (read(fd, (char *)&data_size, 1) != 1) return -1;
    if (read(fd, (char *)(&data_size)+1, 1) != 1) return -1;

    len = (int) ntohs(data_size);

    for (n=0; n < len; n += nr) {
        if ((nr = read(fd, buf+n, len-n)) <= 0)
            return nr;
    }
    return len;
}

int writen(int fd, char *buf, int nbytes)
{
    short data_size = nbytes;
    int n, nw;

    if (nbytes > MAX_BLOCK_SIZE)
        return -3;

    data_size = htons(data_size);
    if (write(fd, (char *)&data_size, 1) != 1) return -1;
    if (write(fd, (char *)(&data_size)+1, 1) != 1) return -1;

    for (n=0; n<nbytes; n += nw) {
        if ((nw = write(fd, buf+n, nbytes-n)) <= 0)
            return nw;
    }
    return n;
}
