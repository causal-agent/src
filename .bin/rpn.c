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

static char *fmt(int radix, int64_t val) {
    static char buf[65];
    if (radix == 2) {
        uint64_t u = val;
        int i = sizeof(buf);
        do {
            buf[--i] = '0' + (u & 1);
        } while (u >>= 1);
        return &buf[i];
    } else if (radix == 8) {
        snprintf(buf, sizeof(buf), "%llo", val);
    } else if (radix == 10) {
        snprintf(buf, sizeof(buf), "%lld", val);
    } else if (radix == 16) {
        snprintf(buf, sizeof(buf), "%llx", val);
    } else abort();
    return buf;
}

static struct {
    int64_t data[1024];
    size_t len;
    int radix;
    char op;
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
    if (stack.op == '@') {
        stack.op = 0;
        while (stack.len > 1)
            stack_op(op);
        return true;
    }
    if (stack.op == '\'') {
        stack.op = 0;
        push(op);
        return true;
    }
    if (stack.op == '"') {
        if (op == '"') stack.op = 0;
        else push(op);
        return true;
    }

    int64_t a, b;
    switch (op) {
        case '@': case '\'': case '"': stack.op = op;
        break; case 'b': stack.radix = 2;
        break; case 'o': stack.radix = 8;
        break; case 'd': stack.radix = 10;
        break; case 'x': stack.radix = 16;
        break; case ';': a = pop();
        break; case ':': a = pop(); push(a); push(a);
        break; case '\\': a = pop(); b = pop(); push(a); push(b);
        break; case '_': a = pop(); push(-a);
        break; case '+': a = pop(); push(pop() + a);
        break; case '-': a = pop(); push(pop() - a);
        break; case '*': a = pop(); push(pop() * a);
        break; case '/': a = pop(); push(pop() / a);
        break; case '%': a = pop(); push(pop() % a);
        break; case '!': a = pop(); push(!a);
        break; case '~': a = pop(); push(~a);
        break; case '&': a = pop(); push(pop() & a);
        break; case '|': a = pop(); push(pop() | a);
        break; case '^': a = pop(); push(pop() ^ a);
        break; case '<': a = pop(); push(pop() << a);
        break; case '>': a = pop(); push(pop() >> a);
        break; case '.': a = pop(); printf("%s\n", fmt(stack.radix, a));
        break; case ',': a = pop(); printf("%c\n", (char) a);
        break; case ' ':
        break; default: return false;
    }
    return true;
}

static char *prompt(EditLine *el __attribute((unused))) {
    static char p[4096];
    if (stack.len == 0) return "[] ";

    size_t q = 0;
    for (size_t i = 0; i < stack.len; ++i) {
        q += (size_t) snprintf(&p[q], sizeof(p) - 2 - q, " %s", fmt(stack.radix, stack.data[i]));
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
    el_set(el, EL_SIGNAL, true);

    for (;;) {
        int count;
        const char *line = el_gets(el, &count);
        if (count < 0) err(EX_IOERR, "el_gets");
        if (!line) break;

        while (*line) {
            if (stack_op(*line)) {
                line++;
            } else {
                char *rest;
                int64_t val = strtoll(line, &rest, stack.radix);
                if (rest != line) {
                    line = rest;
                    push(val);
                } else line++;
            }
        }
    }

    el_end(el);
    return EX_OK;
}
