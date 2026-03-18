#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdint.h>
#include <termios.h>
#include <sys/stat.h>

#define DEFAULT_PORT 35008
#define BACKLOG 8
#define MAX_FRAME 65536
#define USERS_FILE "users.txt"
#define SHELL_PATH "/home/clarence/Desktop/A2/Part_1/myshell"
#define USER_BASE_DIR "/tmp"

enum {
    FT_STDIN = 0x01,
    FT_STDOUT = 0x02,
    FT_STDERR = 0x03,
    FT_WINCH = 0x04,
    FT_EXIT = 0x05,
    FT_AUTH = 0x10,
    FT_AUTH_OK = 0x11,
    FT_AUTH_ERR = 0x12,
    FT_ERROR = 0x20
};

static ssize_t readn(int fd, void *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, (char*)buf + off, n - off);
        if (r == 0) return 0;
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += r;
    }
    return off;
}

static ssize_t writen(int fd, const void *buf, size_t n) {
    size_t off = 0;
    while (off < n) {
        ssize_t r = write(fd, (const char*)buf + off, n - off);
        if (r <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += r;
    }
    return off;
}

static int send_frame(int sock, uint8_t type, const void *payload, uint32_t len) {
    if (len > MAX_FRAME) return -1;
    uint8_t hdr[5];
    hdr[0] = type;
    uint32_t be_len = htonl(len);
    memcpy(hdr + 1, &be_len, 4);
    if (writen(sock, hdr, 5) != 5) return -1;
    if (len > 0 && writen(sock, payload, len) != (ssize_t)len) return -1;
    return 0;
}

static int read_header(int sock, uint8_t *type, uint32_t *len) {
    uint8_t hdr[5];
    ssize_t r = readn(sock, hdr, 5);
    if (r == 0) return 0;
    if (r < 0) return -1;
    *type = hdr[0];
    uint32_t be_len;
    memcpy(&be_len, hdr + 1, 4);
    *len = ntohl(be_len);
    if (*len > MAX_FRAME) return -1;
    return 1;
}

/* Simple authentication */
static int validate_credentials(const char *user, const char *pass) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) {
        perror("open users.txt");
        return -1;
    }
    char *line = NULL;
    size_t sz = 0;
    int ok = -1;
    while (getline(&line, &sz, f) != -1) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (line[0] == '#' || !*line) continue;
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        if (strcmp(line, user) == 0 && strcmp(colon + 1, pass) == 0) {
            ok = 0;
            break;
        }
    }
    free(line);
    fclose(f);
    return ok;
}

/* Handle one client session */
static void handle_client(int cs) {
    uint8_t type;
    uint32_t len;

    /* Read authentication frame */
    if (read_header(cs, &type, &len) <= 0 || type != FT_AUTH) {
        close(cs);
        return;
    }

    char *payload = malloc(len + 1);
    if (!payload) { close(cs); return; }
    if (readn(cs, payload, len) != (ssize_t)len) { free(payload); close(cs); return; }
    payload[len] = '\0';

    char *user = payload;
    char *pass = memchr(payload, '\0', len);
    if (!pass) { free(payload); close(cs); return; }
    pass++;

    char username_copy[64];
    strncpy(username_copy, user, sizeof(username_copy) - 1);
    username_copy[sizeof(username_copy) - 1] = '\0';

    if (validate_credentials(user, pass) != 0) {
        send_frame(cs, FT_AUTH_ERR, "Invalid credentials", 20);
        free(payload);
        close(cs);
        return;
    }

    free(payload);
    send_frame(cs, FT_AUTH_OK, "OK", 2);
    fprintf(stderr, "[SERVER] Authenticated user '%s'\n", username_copy);

    /* Create per-user working directory in /tmp */
    char userdir[256];
    snprintf(userdir, sizeof(userdir), "%s/%s", USER_BASE_DIR, username_copy);
    mkdir(userdir, 0755);  /* create if missing */

    if (chdir(userdir) != 0) {
        perror("chdir userdir");
        send_frame(cs, FT_ERROR, "Failed to set working directory", 32);
    }

    int mfd;
    pid_t pid = forkpty(&mfd, NULL, NULL, NULL);
    if (pid < 0) {
        perror("forkpty");
        close(cs);
        return;
    }

    if (pid == 0) {
        /* Child: run interactive shell */
        execl(SHELL_PATH, "bash", "-i", (char*)NULL);
        perror("execl");
        _exit(127);
    }

    fprintf(stderr, "[SERVER] Spawned shell PID=%d for %s\n", pid, username_copy);

    fd_set fds;
    int maxfd = (mfd > cs ? mfd : cs) + 1;
    char buf[4096];

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(mfd, &fds);
        FD_SET(cs, &fds);
        if (select(maxfd, &fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            break;
        }

        /* Shell -> Client */
        if (FD_ISSET(mfd, &fds)) {
            ssize_t n = read(mfd, buf, sizeof(buf));
            if (n > 0) send_frame(cs, FT_STDOUT, buf, (uint32_t)n);
            else break;
        }

        /* Client -> Shell */
        if (FD_ISSET(cs, &fds)) {
            uint8_t t; uint32_t l;
            int rh = read_header(cs, &t, &l);
            if (rh <= 0) break;
            char *p = NULL;
            if (l > 0) {
                p = malloc(l);
                if (!p) break;
                if (readn(cs, p, l) != (ssize_t)l) { free(p); break; }
            }
            if (t == FT_STDIN && l > 0) write(mfd, p, l);
            free(p);
        }

        /* Child exit check */
        int status;
        pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == pid) break;
    }

    close(mfd);
    close(cs);
    waitpid(pid, NULL, 0);
    fprintf(stderr, "[SERVER] Session for %s closed\n", username_copy);
}

/* Network setup */
static int start_listener(int port) {
    int s = socket(AF_INET6, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }
    if (listen(s, BACKLOG) < 0) {
        perror("listen");
        close(s);
        return -1;
    }
    fprintf(stderr, "[SERVER] Listening on port %d\n", port);
    return s;
}

int main(int argc, char **argv) {
    int port = (argc >= 2) ? atoi(argv[1]) : DEFAULT_PORT;
    int ls = start_listener(port);
    if (ls < 0) return 1;

    for (;;) {
        struct sockaddr_storage ss;
        socklen_t sl = sizeof(ss);
        int cs = accept(ls, (struct sockaddr*)&ss, &sl);
        if (cs < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        pid_t pid = fork();
        if (pid == 0) {
            close(ls);
            handle_client(cs);
            _exit(0);
        } else if (pid > 0) {
            close(cs);
        } else {
            perror("fork");
            close(cs);
        }
    }
}

