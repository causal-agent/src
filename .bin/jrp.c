#if 0
exec cc -Wall -Wextra $@ $0 -ledit -o $(dirname $0)/jrp
#endif

#include <err.h>
#include <histedit.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned long long op;
typedef long long value;
typedef unsigned long long uvalue;
typedef value *(*fptr)(value *);

static char *fmt_bin(value val) {
    static char buf[sizeof(value) * 8 + 1];
    uvalue uval = val;
    int i = sizeof(buf);
    do {
        buf[--i] = '0' + (uval & 1);
    } while (uval >>= 1);
    return &buf[i];
}

static void rt_print_ascii(value val) {
    printf("%c\n", (char)val);
}
static void rt_print_bin(value val) {
    printf("%s\n", fmt_bin(val));
}
static void rt_print_oct(value val) {
    printf("%llo\n", val);
}
static void rt_print_dec(value val) {
    printf("%lld\n", val);
}
static void rt_print_hex(value val) {
    printf("%llx\n", val);
}

enum {
    HOP_NOP  = 0x90666666, // nop
    HOP_DROP = 0x9066665f, // pop rdi
    HOP_DUP  = 0x90666657, // push rdi
    HOP_SWAP = 0x243c8748, // xchg rdi, [rsp]
    HOP_NEG  = 0x90dff748, // neg rdi
    HOP_ADD  = 0xc7014858, // pop rax; add rdi, rax
    HOP_QUO  = 0x90c78948, // mov rdi, rax
    HOP_REM  = 0x90d78948, // mov rdi, rdx
    HOP_NOT  = 0x90d7f748, // not rdi
    HOP_AND  = 0xc7214858, // pop rax; and rdi, rax
    HOP_OR   = 0xc7094858, // pop rax; or rdi, rax
    HOP_XOR  = 0xc7314858, // pop rax; xor rdi, rax
};

enum {
    FOP_PROL = 0x5ffc8948e5894855, // push rbp; mov rbp, rsp; mov rsp, rdi; pop rdi
    FOP_EPIL = 0x5dec8948e0894857, // push rdi; mov rax, rsp; mov rsp, rbp; pop rbp
    FOP_RET  = 0x90666690666666c3, // ret
    FOP_CRT  = 0xb848906666e58748, // xchg rsp, rbp; mov rax, strict qword 0
    FOP_CALL = 0x90665fe58748d0ff, // call rax; xchg rsp, rbp; pop rdi
    FOP_PUSH = 0xbf48909066666657, // push rdi; mov rdi, strict qword 0
    FOP_SUB  = 0x9066665f243c2948, // sub [rsp], rdi; pop rdi
    FOP_MUL  = 0x906666f8af0f4858, // pop rax; imul rdi, rax
    FOP_DIV  = 0x9066fff748994858, // pop rax; cqo; idiv rdi
    FOP_SHL  = 0x5f2424d348f98948, // mov rcx, rdi; shl qword [rsp], cl; pop rdi
    FOP_SHR  = 0x5f242cd348f98948, // mov rcx, rdi; shr qword [rsp], cl; pop rdi
};

static int page;

static int radix = 10;

static struct {
    value *base;
    value *limit;
    value *ptr;
} stack;

static struct {
    op *base;
    op *ptr;
    op hop;
} code;

static void jit_hop(op hop) {
    if (code.hop) {
        *code.ptr++ = hop << 32 | code.hop;
        code.hop = 0;
    } else {
        code.hop = hop;
    }
}

static void jit_fop(op fop) {
    if (code.hop) jit_hop(HOP_NOP);
    *code.ptr++ = fop;
}

static void jit_push(value imm) {
    jit_fop(FOP_PUSH);
    jit_fop((op)imm);
}

static void jit_call(void (*fn)(value)) {
    jit_fop(FOP_CRT);
    jit_fop((op)fn);
    jit_fop(FOP_CALL);
}

static void jit(const char *src) {
    int error;

    code.ptr = code.base;
    jit_fop(FOP_PROL);

    bool quote = false;
    while (*src) {
        if (quote) {
            jit_push(*src++);
            quote = false;
            continue;
        }
        switch (*src) {
            case ' ': break;
            case 39:  quote = true; break;
            case 'b': radix = 2;    break;
            case 'o': radix = 8;    break;
            case 'd': radix = 10;   break;
            case 'x': radix = 16;   break;
            case ';': jit_hop(HOP_DROP); break;
            case ':': jit_hop(HOP_DUP);  break;
            case 92:  jit_hop(HOP_SWAP); break;
            case '_': jit_hop(HOP_NEG);  break;
            case '+': jit_hop(HOP_ADD);  break;
            case '-': jit_fop(FOP_SUB);  break;
            case '*': jit_fop(FOP_MUL);  break;
            case '/': jit_fop(FOP_DIV); jit_hop(HOP_QUO); break;
            case '%': jit_fop(FOP_DIV); jit_hop(HOP_REM); break;
            case '~': jit_hop(HOP_NOT);  break;
            case '&': jit_hop(HOP_AND);  break;
            case '|': jit_hop(HOP_OR);   break;
            case '^': jit_hop(HOP_XOR);  break;
            case '<': jit_fop(FOP_SHL);  break;
            case '>': jit_fop(FOP_SHR);  break;
            case ',': jit_call(rt_print_ascii); break;
            case '.': switch (radix) {
                case 2:  jit_call(rt_print_bin); break;
                case 8:  jit_call(rt_print_oct); break;
                case 10: jit_call(rt_print_dec); break;
                case 16: jit_call(rt_print_hex); break;
            } break;
            default: {
                char *rest;
                value val = strtoll(src, &rest, radix);
                if (rest != src) {
                    src = rest;
                    jit_push(val);
                    continue;
                }
            }
        }
        src++;
    }

    jit_fop(FOP_EPIL);
    jit_fop(FOP_RET);

    error = mprotect(code.base, page, PROT_READ | PROT_EXEC);
    if (error) err(EX_OSERR, "mprotect");

    stack.ptr = ((fptr)code.base)(stack.ptr);

    error = mprotect(code.base, page, PROT_READ | PROT_WRITE);
    if (error) err(EX_OSERR, "mprotect");
}

static char *prompt(EditLine *el __attribute((unused))) {
    static char buf[4096];
    char *bp = buf;
    for (value *sp = stack.limit - 1; sp >= stack.ptr; --sp) {
        size_t len = sizeof(buf) - (buf - bp) - 2;
        switch (radix) {
            case 2:  bp += snprintf(bp, len, " %s", fmt_bin(*sp)); break;
            case 8:  bp += snprintf(bp, len, " %llo", *sp); break;
            case 10: bp += snprintf(bp, len, " %lld", *sp); break;
            case 16: bp += snprintf(bp, len, " %llx", *sp); break;
        }
    }
    buf[0] = '[';
    *bp++ = ']';
    *bp++ = ' ';
    *bp = 0;
    return buf;
}

int main(int argc, char *argv[]) {
    page = getpagesize();

    code.base = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (code.base == MAP_FAILED) err(EX_OSERR, "mmap");

    stack.base = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (stack.base == MAP_FAILED) err(EX_OSERR, "mmap");
    stack.limit = stack.base + page / sizeof(value);
    stack.ptr = stack.limit - 1;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i)
            jit(argv[i]);
        return EX_OK;
    }

    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    el_set(el, EL_PROMPT, prompt);
    el_set(el, EL_SIGNAL, 1);

    for (;;) {
        int count;
        const char *line = el_gets(el, &count);
        if (count < 0) err(EX_IOERR, "el_gets");
        if (!line) break;
        jit(line);
    }

    el_end(el);
    return EX_OK;
}
