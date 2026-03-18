#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <arpa/inet.h>
#include <inttypes.h>

#define DEFAULT_PORT "35008"
#define MAX_FRAME 65536

enum {
    FT_STDIN = 0x01,
    FT_STDOUT = 0x02,
    FT_STDERR = 0x03,
    FT_WINCH = 0x04,
    FT_EXIT = 0x05,
    FT_KEEPALIVE = 0x06,
    FT_AUTH = 0x10,
    FT_AUTH_OK = 0x11,
    FT_AUTH_ERR = 0x12,
    FT_ERROR = 0x20
};

static struct termios orig_term;
static int saved = 0;

static void restore_terminal(void){
    if (saved){
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
        saved = 0;
    }
}
static void enable_raw(void){
    if (saved) return;
    if (tcgetattr(STDIN_FILENO, &orig_term) == -1) return;
    atexit(restore_terminal);
    struct termios t = orig_term;
    cfmakeraw(&t);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    saved = 1;
}

static ssize_t writen(int fd, const void *buf, size_t n){
    size_t off=0;
    while (off < n){
        ssize_t r = write(fd, (const char*)buf + off, n - off);
        if (r <= 0){
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)r;
    }
    return (ssize_t)off;
}
static ssize_t readn(int fd, void *buf, size_t n){
    size_t off=0;
    while (off < n){
        ssize_t r = read(fd, (char*)buf + off, n - off);
        if (r == 0) return 0;
        if (r < 0){
            if (errno==EINTR) continue;
            return -1;
        }
        off += (size_t)r;
    }
    return (ssize_t)off;
}

static int send_frame(int sock, uint8_t type, const void *payload, uint32_t len){
    if (len > MAX_FRAME) return -1;
    uint8_t hdr[5];
    hdr[0] = type;
    uint32_t be_len = htonl(len);
    memcpy(hdr+1, &be_len, 4);
    if (writen(sock, hdr, 5) != 5) return -1;
    if (len > 0){
        if (writen(sock, payload, len) != (ssize_t)len) return -1;
    }
    return 0;
}
static int read_header(int sock, uint8_t *type, uint32_t *len){
    uint8_t hdr[5];
    ssize_t r = readn(sock, hdr, 5);
    if (r == 0) return 0;
    if (r < 0) return -1;
    *type = hdr[0];
    uint32_t be_len;
    memcpy(&be_len, hdr+1, 4);
    *len = ntohl(be_len);
    if (*len > MAX_FRAME) return -1;
    return 1;
}

static void send_winch(int sock){
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0){
        uint32_t r = htonl((uint32_t)ws.ws_row);
        uint32_t c = htonl((uint32_t)ws.ws_col);
        uint8_t buf[8];
        memcpy(buf, &r, 4);
        memcpy(buf+4, &c, 4);
        send_frame(sock, FT_WINCH, buf, 8);
    }
}

volatile sig_atomic_t winch_flag = 0;
void sigwinch_handler(int s){ (void)s; winch_flag = 1; }

int main(int argc, char **argv){
    if (argc < 3){
        fprintf(stderr, "Usage: %s host username [port]\n", argv[0]);
        return 1;
    }
    const char *host = argv[1];
    const char *username = argv[2];
    const char *port = (argc >= 4) ? argv[3] : DEFAULT_PORT;
    char password[256];
    printf("Password: ");
    fflush(stdout);
    if (!fgets(password, sizeof(password), stdin)) return 1;
    char *nl = strchr(password, '\n'); if (nl) *nl = '\0';

    struct addrinfo hints, *res, *rp;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, port, &hints, &res) != 0){
        perror("getaddrinfo");
        return 1;
    }
    int sock = -1;
    for (rp=res; rp; rp = rp->ai_next){
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sock); sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0){ perror("connect"); return 1; }

    /* send AUTH */
    size_t ul = strlen(username);
    size_t pl = strlen(password);
    size_t total = ul + 1 + pl + 1;
    char *payload = malloc(total);
    memcpy(payload, username, ul);
    payload[ul] = '\0';
    memcpy(payload + ul + 1, password, pl);
    payload[ul+1+pl] = '\0';
    if (send_frame(sock, FT_AUTH, payload, (uint32_t)total) != 0){
        fprintf(stderr, "failed to send auth\n");
        free(payload); close(sock); return 1;
    }
    free(payload);

    uint8_t type;
    uint32_t len;
    int rh = read_header(sock, &type, &len);
    if (rh <= 0){ fprintf(stderr, "server closed\n"); close(sock); return 1; }
    char *msg = NULL;
    if (len > 0){
        msg = malloc(len+1);
        if (readn(sock, msg, len) != (ssize_t)len){ free(msg); close(sock); return 1; }
        msg[len] = '\0';
    }
    if (type == FT_AUTH_ERR){
        fprintf(stderr, "Auth failed: %s\n", msg?msg:"(no message)");
        free(msg); close(sock); return 1;
    } else if (type != FT_AUTH_OK){
        fprintf(stderr, "Unexpected reply type %02x\n", type);
        free(msg); close(sock); return 1;
    }
    free(msg);

    enable_raw();
    signal(SIGWINCH, sigwinch_handler);
    send_winch(sock);

    fd_set rfds;
    int maxfd = sock > STDIN_FILENO ? sock : STDIN_FILENO;
    while (1){
        if (winch_flag){
            send_winch(sock);
            winch_flag = 0;
        }
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(sock, &rfds);
        int rv = select(maxfd+1, &rfds, NULL, NULL, NULL);
        if (rv < 0){
            if (errno == EINTR) continue;
            break;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds)){
            char buf[4096];
            ssize_t r = read(STDIN_FILENO, buf, sizeof(buf));
            if (r <= 0) break;
            if (send_frame(sock, FT_STDIN, buf, (uint32_t)r) != 0) break;
        }
        if (FD_ISSET(sock, &rfds)){
            uint8_t rtype;
            uint32_t rlen;
            int rh = read_header(sock, &rtype, &rlen);
            if (rh == 0) { break; }
            if (rh < 0) { fprintf(stderr,"protocol error\n"); break; }
            char *rbuf = NULL;
            if (rlen > 0){
                rbuf = malloc(rlen);
                if (!rbuf) break;
                if (readn(sock, rbuf, rlen) != (ssize_t)rlen){ free(rbuf); break; }
            }
            /* Debug: print received frame type to stderr */
            /* fprintf(stderr, "[CLIENT] recv type=%02x len=%u\n", rtype, rlen); */

            if (rtype == FT_STDOUT){
                if (rlen > 0) writen(STDOUT_FILENO, rbuf, rlen);
            } else if (rtype == FT_STDERR){
                if (rlen > 0) writen(STDERR_FILENO, rbuf, rlen);
            } else if (rtype == FT_EXIT){
                if (rlen >= 4){
                    uint32_t be; memcpy(&be, rbuf, 4);
                    int exitcode = (int)ntohl(be);
                    fprintf(stderr, "\nRemote shell exited with code %d\n", exitcode);
                } else {
                    fprintf(stderr, "\nRemote shell exited\n");
                }
                free(rbuf);
                break;
            } else if (rtype == FT_ERROR || rtype == FT_AUTH_ERR){
                if (rlen > 0) fprintf(stderr, "\nServer error: %.*s\n", (int)rlen, rbuf);
                free(rbuf);
                break;
            }
            free(rbuf);
        }
    }

    restore_terminal();
    close(sock);
    return 0;
}

