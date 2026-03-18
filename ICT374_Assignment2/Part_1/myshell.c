/* myshell.c -- heap-safe UNIX shell implemented in C
 *
 * (patched: fixed redirection parsing for attached forms, added SIGCHLD reaper,
 *  prompt supports multi-word prompts, print background leader pid, minor robustness)
 *
 * Compile:
 *   gcc -std=c99 -Wall -O2 -o myshell myshell.c
 *
 * For debugging:
 *   gcc -std=c99 -Wall -Wextra -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -o myshell myshell.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <termios.h>
#include <ctype.h>

#define MAX_LINE 4096
#define MAX_ARGS 1005
#define MAX_CMDS 100
#define MAX_JOBS 100
#define MAX_HISTORY 1000

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *infile, *outfile, *errfile;
} Command;

typedef struct {
    Command cmds[MAX_CMDS];
    int ncmds;
    int background;
} Job;

static char *prompt = NULL;
static char *history_buf[MAX_HISTORY];
static int hist_count = 0;

/* --- Signal helpers --- */
static void set_ignore_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

/* Reap dead children (for background jobs) */
static void sigchld_handler(int signo) {
    (void)signo;
    int saved_errno = errno;
    while (1) {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if (pid <= 0) break;
    }
    errno = saved_errno;
}

static void set_reap_sigchld(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

static void set_default_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

/* --- Utilities --- */
static char *safe_strdup(const char *s) {
    if (!s) return NULL;
    char *r = strdup(s);
    if (!r) { fprintf(stderr, "out of memory\n"); exit(1); }
    return r;
}

static char *trim(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s) - 1;
    while (e >= s && (*e == ' ' || *e == '\t' || *e == '\n')) { *e = '\0'; e--; }
    return s;
}

static int has_glob(const char *s) {
    if (!s) return 0;
    for (; *s; ++s) if (*s == '*' || *s == '?') return 1;
    return 0;
}

static char **glob_expand(const char *pat, int *out_count) {
    glob_t g;
    if (glob(pat, 0, NULL, &g) != 0) { globfree(&g); if (out_count) *out_count = 0; return NULL; }
    int n = (int)g.gl_pathc;
    if (n == 0) { globfree(&g); if (out_count) *out_count = 0; return NULL; }
    char **arr = malloc((n + 1) * sizeof(char *));
    if (!arr) { globfree(&g); return NULL; }
    for (int i = 0; i < n; ++i) arr[i] = safe_strdup(g.gl_pathv[i]);
    arr[n] = NULL;
    if (out_count) *out_count = n;
    globfree(&g);
    return arr;
}

/* --- History --- */
static void add_history(const char *line) {
    if (!line) return;
    if (hist_count < MAX_HISTORY)
        history_buf[hist_count++] = safe_strdup(line);
    else {
        free(history_buf[0]);
        memmove(history_buf, history_buf + 1, (MAX_HISTORY - 1) * sizeof(char *));
        history_buf[MAX_HISTORY - 1] = safe_strdup(line);
    }
}

static void print_history(void) {
    for (int i = 0; i < hist_count; ++i)
        printf("%4d  %s\n", i + 1, history_buf[i]);
}

static char *hist_expand(const char *line) {
    if (!line || line[0] != '!') return NULL;
    if (strcmp(line, "!!") == 0) {
        if (hist_count == 0) return NULL;
        return safe_strdup(history_buf[hist_count - 1]);
    }
    if (line[1] >= '0' && line[1] <= '9') {
        int n = atoi(line + 1);
        if (n <= 0 || n > hist_count) return NULL;
        return safe_strdup(history_buf[n - 1]);
    }
    const char *prefix = line + 1;
    for (int i = hist_count - 1; i >= 0; --i)
        if (strncmp(history_buf[i], prefix, strlen(prefix)) == 0)
            return safe_strdup(history_buf[i]);
    return NULL;
}

/* --- Tokenizer --- */
static char **tokenize(const char *line, int *out_count) {
    if (!line) { if (out_count) *out_count = 0; return NULL; }
    int cap = 64, n = 0;
    char **toks = malloc(cap * sizeof(char *));
    if (!toks) { fprintf(stderr, "malloc failed\n"); exit(1); }
    const char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p || *p == '\n') break;
        char buf[MAX_LINE]; int bi = 0;
        if (*p == '\'' || *p == '"') {
            char q = *p++;
            while (*p && *p != q) {
                if (*p == '\\' && *(p + 1)) { ++p; buf[bi++] = *p++; }
                else buf[bi++] = *p++;
                if (bi >= MAX_LINE - 1) break;
            }
            if (*p == q) ++p;
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
                if (*p == '\\' && *(p + 1)) { ++p; buf[bi++] = *p++; }
                else buf[bi++] = *p++;
                if (bi >= MAX_LINE - 1) break;
            }
        }
        buf[bi] = '\0';
        toks[n++] = safe_strdup(buf);
        if (n + 1 >= cap) {
            cap *= 2;
            toks = realloc(toks, cap * sizeof(char *));
            if (!toks) { fprintf(stderr, "realloc failed\n"); exit(1); }
        }
    }
    toks[n] = NULL;
    if (out_count) *out_count = n;
    return toks;
}

static void free_tokens(char **toks) {
    if (!toks) return;
    for (int i = 0; toks[i]; ++i) free(toks[i]);
    free(toks);
}

/* --- Parser --- */
static int parse_jobs(const char *line, Job *jobs, int max_jobs) {
    if (!line) return 0;
    int ntok = 0;
    char **toks = tokenize(line, &ntok);
    if (!toks) return 0;

    int jobi = 0;
    memset(&jobs[jobi], 0, sizeof(Job));
    Command cur;
    memset(&cur, 0, sizeof(cur));

    for (int i = 0; i < ntok; ++i) {
        char *tok = toks[i];
        if (strcmp(tok, "|") == 0) {
            if (cur.argc == 0) { fprintf(stderr, "syntax error near '|'\n"); continue; }
            if (jobs[jobi].ncmds < MAX_CMDS)
                jobs[jobi].cmds[jobs[jobi].ncmds++] = cur;
            else
                fprintf(stderr, "too many piped commands\n");
            memset(&cur, 0, sizeof(cur));
        } else if (strcmp(tok, "&") == 0 || strcmp(tok, ";") == 0) {
            if (cur.argc > 0) {
                if (jobs[jobi].ncmds < MAX_CMDS)
                    jobs[jobi].cmds[jobs[jobi].ncmds++] = cur;
            }
            jobs[jobi].background = (strcmp(tok, "&") == 0) ? 1 : 0;
            jobi++;
            if (jobi >= max_jobs) { jobi = max_jobs - 1; break; }
            memset(&jobs[jobi], 0, sizeof(Job));
            memset(&cur, 0, sizeof(cur));
        } else if (
            /* explicit tokens: "<" or ">" or "2>" */
            strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0 || strcmp(tok, "2>") == 0 ||
            /* attached forms: "<file" or ">file" or "2>file" */
            (tok[0] == '<') || (tok[0] == '>' && tok[1] != '\0') || (tok[0] == '2' && tok[1] == '>')
        ) {
            char *target = NULL;
            /* case: token is exactly "<", ">" or "2>" -> filename should be next token */
            if (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0 || strcmp(tok, "2>") == 0) {
                if (i + 1 < ntok) target = toks[++i];
                else { fprintf(stderr, "redirection without filename\n"); continue; }
            } else {
                /* attached forms */
                if (tok[0] == '<') target = tok + 1;          /* "<in" */
                else if (tok[0] == '>' && tok[1] != '\0') target = tok + 1; /* ">out" */
                else if (tok[0] == '2' && tok[1] == '>') target = tok + 2; /* "2>err" or "2>err" */
            }
            if (!target) { fprintf(stderr, "bad redirection\n"); continue; }
            if (tok[0] == '<')
                cur.infile = safe_strdup(target);
            else if (tok[0] == '2')
                cur.errfile = safe_strdup(target);
            else if (tok[0] == '>')
                cur.outfile = safe_strdup(target);
        } else {
            int expanded = 0;
            if (has_glob(tok)) {
                int n;
                char **g = glob_expand(tok, &n);
                if (g) {
                    for (int k = 0; k < n; ++k)
                        if (cur.argc + 1 < MAX_ARGS) cur.argv[cur.argc++] = safe_strdup(g[k]);
                    for (int k = 0; k < n; ++k) free(g[k]);
                    free(g);
                    expanded = 1;
                }
            }
            if (!expanded) {
                if (cur.argc + 1 < MAX_ARGS)
                    cur.argv[cur.argc++] = safe_strdup(tok);
                else fprintf(stderr, "too many args\n");
            }
        }
    }

    if (cur.argc > 0 || cur.infile || cur.outfile || cur.errfile) {
        if (jobs[jobi].ncmds < MAX_CMDS)
            jobs[jobi].cmds[jobs[jobi].ncmds++] = cur;
    }

    free_tokens(toks);
    return jobi + 1;
}

static void free_jobs(Job *jobs, int nj) {
    for (int j = 0; j < nj; ++j)
        for (int c = 0; c < jobs[j].ncmds; ++c) {
            Command *cmd = &jobs[j].cmds[c];
            for (int a = 0; a < cmd->argc; ++a) free(cmd->argv[a]);
            free(cmd->infile);
            free(cmd->outfile);
            free(cmd->errfile);
        }
}

/* --- Builtins --- */
static int handle_builtin_if_any(Command *cmd) {
    if (!cmd || cmd->argc == 0) return 0;
    char *name = cmd->argv[0];
    if (strcmp(name, "exit") == 0) { int s = 0; if (cmd->argc >= 2) s = atoi(cmd->argv[1]); exit(s); }
    if (strcmp(name, "pwd") == 0) { char buf[PATH_MAX]; if (getcwd(buf, sizeof(buf))) puts(buf); else perror("pwd"); return 1; }
    if (strcmp(name, "cd") == 0) { const char *p = (cmd->argc < 2) ? getenv("HOME") : cmd->argv[1]; if (!p) p = "/"; if (chdir(p) != 0) perror("cd"); return 1; }
    if (strcmp(name, "prompt") == 0) {
        if (cmd->argc >= 2) {
            /* join remaining args with spaces to support multi-word prompt */
            size_t tot = 0;
            for (int i = 1; i < cmd->argc; ++i) tot += strlen(cmd->argv[i]) + 1;
            char *buf = malloc(tot + 1);
            if (!buf) { fprintf(stderr, "out of memory\n"); exit(1); }
            buf[0] = '\0';
            for (int i = 1; i < cmd->argc; ++i) {
                if (i > 1) strcat(buf, " ");
                strcat(buf, cmd->argv[i]);
            }
            free(prompt);
            prompt = buf;
        } else fprintf(stderr, "usage: prompt <string>\n");
        return 1;
    }
    if (strcmp(name, "history") == 0) { print_history(); return 1; }
    return 0;
}

/* --- Execution --- */
static void execute_job(Job *job) {
    if (!job || job->ncmds == 0) return;
    if (job->ncmds == 1 && handle_builtin_if_any(&job->cmds[0])) return;

    int n = job->ncmds;
    int (*pipes)[2] = NULL;
    if (n > 1) {
        pipes = malloc((n - 1) * sizeof(int[2]));
        if (!pipes) { perror("malloc"); return; }
        for (int i = 0; i < n - 1; ++i)
            if (pipe(pipes[i]) < 0) { perror("pipe"); free(pipes); return; }
    }

    pid_t *pids = malloc(n * sizeof(pid_t));
    if (!pids) { perror("malloc"); if (pipes) free(pipes); return; }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(pids);
            if (pipes) free(pipes);
            return;
        }
        if (pid == 0) {
            set_default_signals();

            if (i == 0) {
                if (job->cmds[i].infile) {
                    int fd = open(job->cmds[i].infile, O_RDONLY);
                    if (fd < 0) { perror("infile"); _exit(127); }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            } else dup2(pipes[i - 1][0], STDIN_FILENO);

            if (i == n - 1) {
                if (job->cmds[i].outfile) {
                    int fd = open(job->cmds[i].outfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                    if (fd < 0) { perror("outfile"); _exit(127); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
            } else dup2(pipes[i][1], STDOUT_FILENO);

            if (job->cmds[i].errfile) {
                int fd = open(job->cmds[i].errfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                if (fd < 0) { perror("errfile"); _exit(127); }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            if (pipes)
                for (int j = 0; j < n - 1; ++j) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

            Command *sc = &job->cmds[i];
            sc->argv[sc->argc] = NULL;
            execvp(sc->argv[0], sc->argv);
            fprintf(stderr, "exec failed: %s: %s\n", sc->argv[0], strerror(errno));
            _exit(127);
        } else pids[i] = pid;
    }

    if (pipes) {
        for (int j = 0; j < n - 1; ++j) { close(pipes[j][0]); close(pipes[j][1]); }
        free(pipes);
    }

    if (job->background) {
        /* print the pipeline leader PID (first child) */
        printf("[bg] %d\n", (int)pids[0]);
        free(pids);
        return;
    }

    for (int i = 0; i < n; ++i) waitpid(pids[i], NULL, 0);
    free(pids);
}

/* --- Interactive input with arrow-key history --- */

static struct termios orig_term;

static void enable_raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_term);
    raw = orig_term;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

/* Simple line editor: supports backspace, Enter, and ↑/↓ for history */
static int read_line_interactive(char *buf, size_t size) {
    enable_raw_mode();
    int hist_index = hist_count;
    size_t len = 0;
    buf[0] = '\0';

    while (1) {
        int c = getchar();
        if (c == EOF) { disable_raw_mode(); return 0; }

        if (c == '\n' || c == '\r') {
            putchar('\n');
            break;
        } else if (c == 127 || c == '\b') {  // Backspace
            if (len > 0) {
                len--;
                buf[len] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (c == 27) { // Escape sequence (arrows)
            int c1 = getchar();
            if (c1 == '[') {
                int c2 = getchar();
                if (c2 == 'A') { // Up arrow
                    if (hist_index > 0) hist_index--;
                    if (hist_index < hist_count) {
                        printf("\33[2K\r%s%s", prompt, history_buf[hist_index]);
                        fflush(stdout);
                        strncpy(buf, history_buf[hist_index], size - 1);
                        buf[size - 1] = '\0';
                        len = strlen(buf);
                    }
                } else if (c2 == 'B') { // Down arrow
                    if (hist_index < hist_count) hist_index++;
                    if (hist_index == hist_count) {
                        printf("\33[2K\r%s", prompt);
                        fflush(stdout);
                        buf[0] = '\0';
                        len = 0;
                    } else {
                        printf("\33[2K\r%s%s", prompt, history_buf[hist_index]);
                        fflush(stdout);
                        strncpy(buf, history_buf[hist_index], size - 1);
                        buf[size - 1] = '\0';
                        len = strlen(buf);
                    }
                }
            }
        } else if (isprint(c)) {
            if (len + 1 < size) {
                buf[len++] = (char)c;
                buf[len] = '\0';
                putchar(c);
                fflush(stdout);
            }
        }
    }

    disable_raw_mode();
    return 1;
}

/* --- Main --- */
int main(void) {
    set_ignore_signals();
    set_reap_sigchld();
    prompt = safe_strdup("% ");
    char linebuf[MAX_LINE];

        while (1) {
        printf("%s", prompt);
        fflush(stdout);

        if (!isatty(STDIN_FILENO)) {
            if (!fgets(linebuf, sizeof(linebuf), stdin)) { printf("\n"); break; }
        } else {
            if (!read_line_interactive(linebuf, sizeof(linebuf))) { printf("\n"); break; }
        }

        char *ln = trim(linebuf);
        if (!ln || ln[0] == '\0') continue;

        if (ln[0] == '!') {
            char *ex = hist_expand(ln);
            if (!ex) { fprintf(stderr, "history: event not found\n"); continue; }
            printf("%s\n", ex);
            strncpy(linebuf, ex, sizeof(linebuf) - 1);
            linebuf[sizeof(linebuf) - 1] = '\0';
            free(ex);
            ln = trim(linebuf);
            add_history(ln);
        } else add_history(ln);

        Job *jobs = calloc(MAX_JOBS, sizeof(Job));
        if (!jobs) { fprintf(stderr, "malloc jobs failed\n"); continue; }
        int nj = parse_jobs(ln, jobs, MAX_JOBS);
        for (int j = 0; j < nj; ++j) {
            if (jobs[j].ncmds == 1 && handle_builtin_if_any(&jobs[j].cmds[0])) continue;
            execute_job(&jobs[j]);
        }
        free_jobs(jobs, nj);
        free(jobs);
    }

    free(prompt);
    for (int i = 0; i < hist_count; ++i) free(history_buf[i]);
    return 0;
}

