#include <err.h>
#include <histedit.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sysexits.h>
#include <unistd.h>

#define UNUSED __attribute__((unused))
#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

static int cdBuiltin(int argc, const char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : getenv("HOME");
    int error = chdir(path);
    if (error) warn("%s", path);
    return error;
}

static int execBuiltin(int argc, const char *argv[]) {
    if (argc < 2) return 0;
    execvp(argv[1], (char *const *)argv + 1);
    err(EX_UNAVAILABLE, "%s", argv[1]);
}

static const struct {
    const char *name;
    int (*fn)(int, const char **);
} BUILTINS[] = {
    { "cd", cdBuiltin },
    { "exec", execBuiltin },
};

static int forkExec(int argc UNUSED, const char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) err(EX_OSERR, "fork");

    if (!pid) {
        execvp(argv[0], (char *const *)argv);
        err(EX_UNAVAILABLE, "%s", argv[0]);
    }

    int status;
    pid_t dead = wait(&status);
    if (dead < 0) err(EX_OSERR, "wait");

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        warnx("%s %s", sys_siglist[WTERMSIG(status)], argv[0]);
        return 128 + WTERMSIG(status);
    }
}

static char *prompt(EditLine *editLine UNUSED) {
    return "$ ";
}

static char *rprompt(EditLine *editLine UNUSED) {
    static char buf[PATH_MAX];
    char *cwd = getcwd(buf, sizeof(buf));
    if (!cwd) err(EX_OSERR, "getcwd");
    return cwd;
}

int main(int argc, const char *argv[]) {
    EditLine *editLine = el_init(argv[0], stdin, stdout, stderr);
    History *hist = history_init();
    Tokenizer *tokenizer = tok_init(NULL);

    el_set(editLine, EL_SIGNAL, 1);
    el_set(editLine, EL_HIST, history, hist);
    el_set(editLine, EL_PROMPT, prompt);
    el_set(editLine, EL_RPROMPT, rprompt);

    HistEvent histEvent;
    history(hist, &histEvent, H_SETSIZE, 1000);
    history(hist, &histEvent, H_SETUNIQUE, 1);
    // TODO: History persistence.

    el_source(editLine, NULL);

    for (;;) {
        int count;
        const char *line = el_gets(editLine, &count);
        if (count < 0) err(EX_IOERR, "el_gets");
        if (!line) break;

        history(hist, &histEvent, H_ENTER, line);

        int tok = tok_line(tokenizer, el_line(editLine), &argc, &argv, NULL, NULL);
        if (tok < 0) errx(EX_SOFTWARE, "tok_line");
        if (tok > 0) continue; // TODO: Change prompt.
        if (!argc) continue;

        bool builtin = false;
        for (size_t i = 0; i < ARRAY_LEN(BUILTINS); ++i) {
            if (strcmp(argv[0], BUILTINS[i].name)) continue;
            builtin = true;
            BUILTINS[i].fn(argc, argv);
            break;
        }
        if (!builtin) forkExec(argc, argv);

        tok_reset(tokenizer);
    }

    el_end(editLine);
    history_end(hist);
    tok_end(tokenizer);
    return EX_OK;
}
