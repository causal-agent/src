#if 0
exec cc -Wall -Wextra $@ -ledit -o $(dirname $0)/rpn $0
#endif

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <histedit.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

static struct {
    int64_t data[1024];
    size_t len;
    int radix;
} stack = { .radix = 10 };

static void push(int64_t val) {
    stack.data[stack.len++] = val;
    assert(stack.len < sizeof(stack.data));
}

static int64_t pop(void) {
    if (stack.len == 0) return 0;
    return stack.data[--stack.len];
}

static bool stack_op(char op) {
    int64_t a, b;
    switch (op) {
        case '$': pop();
        break; case '\\': a = pop(); b = pop(); push(a); push(b);
        break; case ':': a = pop(); push(a); push(a);
        break; case '_': a = pop(); push(-a);
        break; case '+': a = pop(); push(pop() + a);
        break; case '-': a = pop(); push(pop() - a);
        break; case '*': a = pop(); push(pop() * a);
        break; case '/': a = pop(); push(pop() / a);
        break; case '%': a = pop(); push(pop() % a);
        break; case '~': a = pop(); push(~a);
        break; case '&': a = pop(); push(pop() & a);
        break; case '|': a = pop(); push(pop() | a);
        break; case '^': a = pop(); push(pop() ^ a);
        break; case '<': a = pop(); push(pop() << a);
        break; case '>': a = pop(); push(pop() >> a);
        break; case '.': a = pop(); printf("%lld\n", a);
        break; case ',': a = pop(); printf("%c\n", (char) a);
        break; default: return false;
    }
    return true;
}

static char *prompt(EditLine *el __attribute((unused))) {
    static char p[4096];
    if (stack.len == 0) return "[] ";

    size_t q = 0;
    for (size_t i = 0; i < stack.len; ++i) {
        q += (size_t) snprintf(&p[q], sizeof(p) - 3 - q, " %lld", stack.data[i]);
    }
    p[0] = '[';
    p[q] = ']';
    p[++q] = ' ';
    p[++q] = 0;

    return p;
}

int main(int argc __attribute((unused)), char *argv[]) {
    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    el_set(el, EL_PROMPT, prompt);

    for (;;) {
        int count;
        const char *line = el_gets(el, &count);
        if (count < 0) err(EX_IOERR, "el_gets");
        if (!line) break;

        while (*line) {
            char *rest;
            int64_t val = strtoll(line, &rest, stack.radix);

        while (*line) {
            if (isdigit(*line))
                push(strtoll(line, (char **) &line, 0)); // XXX: ???
            else
                stack_op(*line++);
        }
    }
    putchar('\n');

    el_end(el);
    return EX_OK;
}
