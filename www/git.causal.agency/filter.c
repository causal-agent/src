#include <err.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define Q(...) #__VA_ARGS__

#define MANDOC_OPTIONS "fragment,man=%N.%S,includes=../tree/%I"

static int about(int argc, char *argv[]) {
	if (argc < 2) return 1;
	if (!fnmatch("README.[1-9]", argv[1], 0)) {
		execlp("mandoc", "mandoc", "-T", "html", "-O", MANDOC_OPTIONS, NULL);
		err(127, "mandoc");
	} else if (!fnmatch("*.[1-9]", argv[1], 0)) {
		execlp(
			"mandoc", "mandoc", "-T", "html", "-O", "toc," MANDOC_OPTIONS, NULL
		);
		err(127, "mandoc");
	} else {
		execlp("hilex", "hilex", "-l", "text", "-f", "html", "-o", "pre", NULL);
		err(127, "hilex");
	}
}

static int email(void) {
	size_t cap = 0;
	char *buf = NULL;
	if (getline(&buf, &cap, stdin) < 0) err(1, "getline");
	if (buf[0] == 'C') {
		printf("C.%s", buf + strcspn(buf, " "));
	} else {
		printf("%s", buf);
	}
	return 0;
}

static int owner(void) {
	printf(Q(<a href="https://liberapay.com/june/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a>));
	return 0;
}

#define CTAGS_PATTERN "*.[chlmy]"
#define TEMPLATE "/tmp/filter.XXXXXXXXXX"

static char tmp[PATH_MAX];
static char tags[] = TEMPLATE;
static void cleanup(void) {
	unlink(tmp);
	unlink(tags);
}

static int source(int argc, char *argv[]) {
	if (argc < 2) return 1;
	if (
		strcmp("Makefile", argv[1]) &&
		strcmp(".profile", argv[1]) &&
		strcmp(".shrc", argv[1]) &&
		fnmatch(CTAGS_PATTERN, argv[1], 0) &&
		fnmatch("*.mk", argv[1], 0) &&
		fnmatch("*.[1-9]", argv[1], 0) &&
		fnmatch("*.sh", argv[1], 0)
	) {
		execlp("hilex", "hilex", "-t", "-n", argv[1], "-f", "html", NULL);
		err(127, "hilex");
	}

	const char *ext = strrchr(argv[1], '.');
	if (!strcmp(argv[1], ".profile") || !strcmp(argv[1], ".shrc")) {
		ext = ".sh";
	} else if (!strcmp(argv[1], "Makefile")) {
		ext = ".mk";
	} else if (!ext) {
		ext = "";
	}

	snprintf(tmp, sizeof(tmp), TEMPLATE "%s", ext);
	int fd = mkstemps(tmp, strlen(ext));
	if (fd < 0) err(1, "%s", tmp);
	atexit(cleanup);

	char buf[4096];
	for (ssize_t len; 0 < (len = read(STDIN_FILENO, buf, sizeof(buf)));) {
		if (write(fd, buf, len) < 0) err(1, "%s", tmp);
	}
	if (close(fd) < 0) err(1, "%s", tmp);

	fd = mkstemp(tags);
	if (fd < 0) err(1, "%s", tags);
	close(fd);
	pid_t pid = fork();
	if (pid < 0) err(1, "fork");
	if (!pid) {
		if (!fnmatch(CTAGS_PATTERN, argv[1], 0)) {
			execlp("ctags", "ctags", "-w", "-f", tags, tmp, NULL);
			warn("ctags");
		} else {
			execlp("mtags", "mtags", "-f", tags, tmp, NULL);
			warn("mtags");
		}
		_exit(127);
	}
	int status;
	if (wait(&status) < 0) err(1, "wait");

	int rw[2];
	if (pipe(rw) < 0) err(1, "pipe");
	pid = fork();
	if (pid < 0) err(1, "fork");
	if (!pid) {
		dup2(rw[1], STDOUT_FILENO);
		close(rw[0]);
		close(rw[1]);
		execlp("hilex", "hilex", "-f", "html", tmp, NULL);
		warn("hilex");
		_exit(127);
	}
	pid = fork();
	if (pid < 0) err(1, "fork");
	if (!pid) {
		dup2(rw[0], STDIN_FILENO);
		close(rw[0]);
		close(rw[1]);
		execlp("htagml", "htagml", "-im", "-f", tags, tmp, NULL);
		warn("htagml");
		_exit(127);
	}
	close(rw[0]);
	close(rw[1]);

	if (wait(&status) < 0) err(1, "wait");
	if (wait(&status) < 0) err(1, "wait");
	return status;
}

int main(int argc, char *argv[]) {
	int error;
	switch (getprogname()[0]) {
		break; case 'a': error = pledge("stdio exec", NULL);
		break; case 's': error = pledge("stdio tmppath proc exec", NULL);
		break; default:  error = pledge("stdio", NULL);
	}
	if (error) err(1, "pledge");
	switch (getprogname()[0]) {
		case 'a': return about(argc, argv);
		case 'e': return email();
		case 'o': return owner();
		case 's': return source(argc, argv);
		default: return 1;
	}
}
