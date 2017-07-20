#if 0
set -e
bin=$(dirname $0)
cc -Wall -Wextra -pedantic $@ -lutil -o $bin/dtch $0
ln -f $bin/dtch $bin/atch
exit
#endif

// Session attach and detach.

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
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

static struct passwd *getUser(void) {
    uid_t uid = getuid();
    struct passwd *user = getpwuid(uid);
    if (!user) err(EX_OSFILE, "/etc/passwd");
    return user;
}

static struct sockaddr_un sockAddr(const char *home, const char *name) {
    struct sockaddr_un addr = { .sun_family = AF_LOCAL };
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/.dtch/%s", home, name);
    return addr;
}

static ssize_t writeAll(int fd, const char *buf, size_t len) {
    ssize_t writeLen;
    while (0 < (writeLen = write(fd, buf, len))) {
        buf += writeLen;
        len -= writeLen;
    }
    return writeLen;
}

char z;
struct iovec iov = { .iov_base = &z, .iov_len = 1 };

static ssize_t sendFd(int sock, int fd) {
    size_t len = CMSG_LEN(sizeof(int));
    char buf[len];
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = buf,
        .msg_controllen = len,
    };

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = len;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = fd;

    return sendmsg(sock, &msg, 0);
}

static int recvFd(int sock) {
    size_t len = CMSG_LEN(sizeof(int));
    char buf[len];
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = buf,
        .msg_controllen = len,
    };

    ssize_t n = recvmsg(sock, &msg, 0);
    if (n < 0) return -1;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) { errno = ENOMSG; return -1; }
    if (cmsg->cmsg_type != SCM_RIGHTS) { errno = EBADMSG; return -1; }

    return *(int *)CMSG_DATA(cmsg);
}

static struct sockaddr_un addr;

static void unlinkAddr(void) {
    unlink(addr.sun_path);
}

static int dtch(int argc, char *argv[]) {
    int error;

    struct passwd *user = getUser();

    char *name = user->pw_name;
    char *cmd = user->pw_shell;
    if (argc > 2) {
        name = argv[1];
        cmd = argv[2];
        argv += 2;
    } else if (argc > 1) {
        name = argv[1];
        argv++;
    }

    int home = open(user->pw_dir, 0);
    if (home < 0) err(EX_IOERR, "%s", user->pw_dir);
    error = mkdirat(home, ".dtch", S_IRWXU);
    if (error && errno != EEXIST) err(EX_IOERR, "%s/.dtch", user->pw_dir);
    error = close(home);
    if (error) err(EX_IOERR, "%s", user->pw_dir);

    int server = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (server < 0) err(EX_OSERR, "socket");

    addr = sockAddr(user->pw_dir, name);
    error = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_IOERR, "%s", addr.sun_path);
    atexit(unlinkAddr);

    error = chmod(addr.sun_path, 0600);
    if (error) err(EX_IOERR, "%s", addr.sun_path);

    int master;
    pid_t pid = forkpty(&master, NULL, NULL, NULL);
    if (pid < 0) err(EX_OSERR, "forkpty");

    if (!pid) {
        error = close(server);
        if (error) warn("close(%d)", server);
        execvp(cmd, argv);
        err(EX_OSERR, "%s", cmd);
    }

    error = listen(server, 0);
    if (error) err(EX_OSERR, "listen");

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) err(EX_IOERR, "accept(%d)", server);

        ssize_t len = sendFd(client, master);
        if (len < 0) warn("sendmsg(%d)", client);

        len = recv(client, &z, sizeof(z), 0);
        if (len < 0) warn("recv(%d)", client);

        error = close(client);
        if (error) warn("close(%d)", client);

        pid_t dead = waitpid(pid, NULL, WNOHANG);
        if (dead < 0) warn("waitpid(%d)", pid);
        if (dead) exit(EX_OK);
    }
}

static struct termios saveTerm;

static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
    printf(
        "\x1b[?1049l" // rmcup
        "\x1b\x63\x1b[!p\x1b[?3;4l\x1b[4l\x1b>" // reset
    );
}

static int atch(int argc, char *argv[]) {
    int error;

    struct passwd *user = getUser();
    char *name = (argc > 1) ? argv[1] : user->pw_name;

    int client = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (client < 0) err(EX_OSERR, "socket");

    struct sockaddr_un addr = sockAddr(user->pw_dir, name);
    error = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_IOERR, "%s", addr.sun_path);

    int master = recvFd(client);
    if (master < 0) err(EX_IOERR, "recvmsg(%d)", client);

    struct winsize window;
    error = ioctl(STDERR_FILENO, TIOCGWINSZ, &window);
    if (error) err(EX_IOERR, "ioctl(%d, TIOCGWINSZ)", STDERR_FILENO);

    error = ioctl(master, TIOCSWINSZ, &window);
    if (error) err(EX_IOERR, "ioctl(%d, TIOCSWINSZ)", master);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_IOERR, "tcgetattr(%d)", STDERR_FILENO);
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr(%d)", STDERR_FILENO);

    char ctrlL = CTRL('L');
    ssize_t len = write(master, &ctrlL, 1);
    if (len < 0) err(EX_IOERR, "write(%d)", master);

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

            if (len && buf[0] == CTRL('Q')) exit(EX_OK);

            len = writeAll(master, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", master);
        }

        if (fds[1].revents) {
            len = read(master, buf, sizeof(buf));
            if (len < 0) err(EX_IOERR, "read(%d)", master);
            len = writeAll(STDOUT_FILENO, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", STDOUT_FILENO);
        }
    }
    err(EX_IOERR, "poll([%d,%d])", STDIN_FILENO, master);
}

int main(int argc, char *argv[]) {
    switch (argv[0][0]) {
        case 'd': return dtch(argc, argv);
        case 'a': return atch(argc, argv);
    }
    return EX_USAGE;
}
