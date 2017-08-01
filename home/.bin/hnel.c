#if 0
exec cc -Wall -Wextra -pedantic $@ -lutil -o $(dirname $0)/hnel $0
#endif

// PTY wrapper for preserving HJKL in Tarmak layouts.

#include <err.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#if defined __FreeBSD__
#include <libutil.h>
#elif defined __linux__
#include <pty.h>
#else
#include <util.h>
#endif

static char table[256] = {
    ['n'] = 'j', ['N'] = 'J', [CTRL('N')] = CTRL('J'),
    ['e'] = 'k', ['E'] = 'K', [CTRL('E')] = CTRL('K'),
    ['j'] = 'e', ['J'] = 'E', [CTRL('J')] = CTRL('E'),
    ['k'] = 'n', ['K'] = 'N', [CTRL('K')] = CTRL('N'),
};

static ssize_t writeAll(int fd, const char *buf, size_t len) {
    ssize_t writeLen;
    while (0 < (writeLen = write(fd, buf, len))) {
        buf += writeLen;
        len -= writeLen;
    }
    return writeLen;
}

static struct termios saveTerm;

static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

int main(int argc, char *argv[]) {
    int error;

    if (argc < 2) return EX_USAGE;

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr");
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr");

    struct winsize window;
    error = ioctl(STDERR_FILENO, TIOCGWINSZ, &window);
    if (error) err(EX_IOERR, "ioctl(%d, TIOCGWINSZ)", STDERR_FILENO);

    int master;
    pid_t pid = forkpty(&master, NULL, NULL, &window);
    if (pid < 0) err(EX_OSERR, "forkpty");

    if (!pid) {
        execvp(argv[1], argv + 1);
        err(EX_OSERR, "%s", argv[1]);
    }

    bool enable = true;

    struct pollfd fds[2] = {
        { .fd = STDIN_FILENO, .events = POLLIN },
        { .fd = master, .events = POLLIN },
    };
    while (0 < poll(fds, 2, -1)) {
        char buf[4096];
        ssize_t len;

        if (fds[0].revents) {
            len = read(STDIN_FILENO, buf, sizeof(buf));
            if (len < 0) err(EX_IOERR, "read(%d)", STDIN_FILENO);

            if (len == 1) {
                if (buf[0] == CTRL('S')) {
                    enable = !enable;
                    continue;
                }

                unsigned char c = buf[0];
                if (enable && table[c]) buf[0] = table[c];
            }

            len = writeAll(master, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", master);
        }

        if (fds[1].revents) {
            len = read(master, buf, sizeof(buf));
            if (len < 0) err(EX_IOERR, "read(%d)", master);
            len = writeAll(STDOUT_FILENO, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", STDOUT_FILENO);
        }

        int status;
        pid_t dead = waitpid(pid, &status, WNOHANG);
        if (dead < 0) err(EX_OSERR, "waitpid(%d)", pid);
        if (dead) return WIFEXITED(status) ? WEXITSTATUS(status) : EX_SOFTWARE;
    }
    err(EX_IOERR, "poll");
}
