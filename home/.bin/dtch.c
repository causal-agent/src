#if 0
set -e
bin=$(dirname $0)
cc -Wall -Wextra -pedantic $@ -o $bin/dtch $0
ln -f $bin/dtch $bin/atch
exit
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>
#include <util.h>

static struct passwd *getUser(void) {
    uid_t uid = getuid();
    struct passwd *user = getpwuid(uid);
    if (!user) err(EX_OSFILE, "/etc/passwd");
    return user;
}

static void createDir(const char *home) {
    int fd = open(home, 0);
    if (fd < 0) err(EX_IOERR, "%s", home);

    int error = mkdirat(fd, ".dtch", S_IRWXU);
    if (error && errno != EEXIST) err(EX_IOERR, "%s/.dtch", home);

    error = close(fd);
    if (error) err(EX_IOERR, "%s", home);
}

static struct sockaddr_un sockAddr(const char *home, const char *name) {
    struct sockaddr_un addr = { .sun_family = AF_LOCAL };
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/.dtch/%s", home, name);
    return addr;
}

static char z;
static struct iovec iov = {
    .iov_base = &z,
    .iov_len = sizeof(z),
};

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
    if (!cmsg) {
        errno = ENOMSG;
        return -1;
    }
    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        errno = EBADMSG;
        return -1;
    }

    return *(int *)CMSG_DATA(cmsg);
}

static ssize_t writeAll(int fd, const char *buf, ssize_t len) {
    ssize_t writeLen;
    while (0 < (writeLen = write(fd, buf, len))) {
        buf += writeLen;
        len -= writeLen;
    }
    return writeLen;
}

static struct sockaddr_un addr;

static void unlinkAddr(void) {
    unlink(addr.sun_path);
}

static int dtch(int argc, char *argv[]) {
    int error;

    const struct passwd *user = getUser();
    const char *name = user->pw_name;
    const char *cmd = user->pw_shell;

    if (argc > 2) {
        name = argv[1];
        cmd = argv[2];
        argv += 2;
    } else if (argc > 1) {
        name = argv[1];
        argv++;
    }

    int server = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (server < 0) err(EX_IOERR, "socket");

    createDir(user->pw_dir);
    addr = sockAddr(user->pw_dir, name);

    error = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_IOERR, "%s", addr.sun_path);
    atexit(unlinkAddr);

    error = chmod(addr.sun_path, S_IRWXU);
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

    // TODO: Kill child on error after this point?

    error = listen(server, 1);
    if (error) err(EX_IOERR, "listen");

    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) err(EX_IOERR, "accept");

        ssize_t len = sendFd(client, master);
        if (len < 0) warn("sendmsg(%d)", client);

        // TODO: Send SIGWINCH?

        len = recv(client, &z, sizeof(z), 0);
        if (len < 0) warn("recv(%d)", client);

        error = close(client);
        if (error) warn("close(%d)", client);

        // TODO: Detect death of child.
    }
}

static struct termios saveTerm;

static void restoreTerm(void) {
    tcsetattr(STDERR_FILENO, TCSADRAIN, &saveTerm);
}

static int atch(int argc, char *argv[]) {
    int error;

    const struct passwd *user = getUser();

    const char *name = user->pw_name;
    if (argc > 1) name = argv[1];

    int client = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (client < 0) err(EX_IOERR, "socket");

    addr = sockAddr(user->pw_dir, name);
    error = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (error) err(EX_IOERR, "%s", addr.sun_path);

    int master = recvFd(client);
    if (master < 0) err(EX_IOERR, "recvmsg(%d)", client);

    error = tcgetattr(STDERR_FILENO, &saveTerm);
    if (error) err(EX_USAGE, "tcgetattr(%d)", STDERR_FILENO);
    atexit(restoreTerm);

    struct termios raw;
    cfmakeraw(&raw);
    error = tcsetattr(STDERR_FILENO, TCSADRAIN, &raw);
    if (error) err(EX_IOERR, "tcsetattr(%d)", STDERR_FILENO);

    struct winsize window;
    error = ioctl(STDERR_FILENO, TIOCGWINSZ, &window);
    if (error) warn("ioctl(%d, TIOCGWINSZ)", STDERR_FILENO);
    error = ioctl(master, TIOCSWINSZ, &window);
    if (error) warn("ioctl(%d, TIOCSWINSZ)", master);

    // TODO: Handle SIGWINCH?

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = master;
    fds[1].events = POLLIN;

    char buf[4096];
    while (0 < poll(fds, 2, -1)) {
        if (fds[0].revents) {
            ssize_t len = read(STDIN_FILENO, buf, sizeof(buf));
            if (len < 0) err(EX_IOERR, "read(%d)", STDIN_FILENO);

            if (len && buf[0] == ('Q' & ~0x40)) {
                // TODO: Message?
                exit(0);
            }

            len = writeAll(master, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", master);
        }

        if (fds[1].revents) {
            ssize_t len = read(master, buf, sizeof(buf));
            if (len < 0) err(EX_IOERR, "read(%d)", master);
            len = writeAll(STDOUT_FILENO, buf, len);
            if (len < 0) err(EX_IOERR, "write(%d)", STDOUT_FILENO);
        }

    }
    err(EX_IOERR, "poll");
}

int main(int argc, char *argv[]) {
    switch (argv[0][0]) {
        case 'd': return dtch(argc, argv);
        case 'a': return atch(argc, argv);
    }
    return EX_USAGE;
}
