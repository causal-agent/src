#if 0
exec cc -Wall -Wextra -pedantic $@ $0 -ledit -o $(dirname $0)/jrp
#endif

#include <err.h>
#include <histedit.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned long qop;
typedef unsigned int dop;

typedef long qvalue;
typedef int dvalue;
typedef unsigned long uvalue;

typedef qvalue *(*jitFn)(qvalue *);
typedef void (*rtFn)(qvalue);

static char *formatBin(qvalue val) {
    static char buf[sizeof(qvalue) * 8 + 1];
    uvalue uval = val;
    size_t i = sizeof(buf);
    do {
        buf[--i] = '0' + (uval & 1);
    } while (uval >>= 1);
    return &buf[i];
}

static void rtPrintAscii(qvalue val) {
    printf("%c\n", (char)val);
}
static void rtPrintBin(qvalue val) {
    printf("%s\n", formatBin(val));
}
static void rtPrintOct(qvalue val) {
    printf("%lo\n", val);
}
static void rtPrintDec(qvalue val) {
    printf("%ld\n", val);
}
static void rtPrintHex(qvalue val) {
    printf("%lx\n", val);
}

static const dop DOP_NOP  = 0x90666666; // nop
static const dop DOP_PUSH = 0xc7c74857; // push rdi; mov rdi, strict dword 0
static const dop DOP_DROP = 0x9066665f; // pop rdi
static const dop DOP_DUP  = 0x90666657; // push rdi
static const dop DOP_SWAP = 0x243c8748; // xchg rdi, [rsp]
static const dop DOP_NEG  = 0x90dff748; // neg rdi
static const dop DOP_ADD  = 0xc7014858; // pop rax; add rdi, rax
static const dop DOP_QUO  = 0x90c78948; // mov rdi, rax
static const dop DOP_REM  = 0x90d78948; // mov rdi, rdx
static const dop DOP_NOT  = 0x90d7f748; // not rdi
static const dop DOP_AND  = 0xc7214858; // pop rax; and rdi, rax
static const dop DOP_OR   = 0xc7094858; // pop rax; or rdi, rax
static const dop DOP_XOR  = 0xc7314858; // pop rax; xor rdi, rax

static const qop QOP_PROL = 0x5ffc8948e5894855; // push rbp; mov rbp, rsp; mov rsp, rdi; pop rdi
static const qop QOP_EPIL = 0x5dec8948e0894857; // push rdi; mov rax, rsp; mov rsp, rbp; pop rbp
static const qop QOP_RET  = 0x90666666906666c3; // ret
static const qop QOP_CRT  = 0xb848906690e58748; // xchg rsp, rbp; mov rax, strict qword 0
static const qop QOP_CALL = 0x90665fe58748d0ff; // call rax; xchg rsp, rbp; pop rdi
static const qop QOP_PUSH = 0xbf48906690666657; // push rdi; mov rdi, strict qword 0
static const qop QOP_SUB  = 0x9066665f243c2948; // sub [rsp], rdi; pop rdi
static const qop QOP_MUL  = 0x906666f8af0f4858; // pop rax; imul rdi, rax
static const qop QOP_DIV  = 0x9066fff748994858; // pop rax; cqo; idiv rdi
static const qop QOP_SHL  = 0x5f2424d348f98948; // mov rcx, rdi; shl qword [rsp], cl; pop rdi
static const qop QOP_SHR  = 0x5f242cd348f98948; // mov rcx, rdi; shr qword [rsp], cl; pop rdi

static int radix = 10;

static struct {
    qop *base;
    qop *ptr;
    dop dop;
} code;

static struct {
    qvalue *base;
    qvalue *limit;
    qvalue *ptr;
} stack;

static void jitDop(dop op) {
    if (code.dop) {
        *code.ptr++ = (qop)op << 32 | code.dop;
        code.dop = 0;
    } else {
        code.dop = op;
    }
}

static void jitQop(qop op) {
    if (code.dop) jitDop(DOP_NOP);
    *code.ptr++ = op;
}

static void jitPush(qvalue imm) {
    if ((dvalue)imm == imm) {
        jitQop(DOP_PUSH | (qop)imm << 32);
    } else {
        jitQop(QOP_PUSH);
        jitQop((qop)imm);
    }
}

static void jitCall(rtFn fn) {
    jitQop(QOP_CRT);
    jitQop((qop)fn);
    jitQop(QOP_CALL);
}

static void jitBegin(void) {
    code.ptr = code.base;
    jitQop(QOP_PROL);
}

static void jitEnd(void) {
    jitQop(QOP_EPIL);
    jitQop(QOP_RET);
}

static void jitInit(void) {
    code.base = mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (code.base == MAP_FAILED) err(EX_OSERR, "mmap");
}

static void stackInit(void) {
    stack.base = mmap(0, 2 * getpagesize(), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (stack.base == MAP_FAILED) err(EX_OSERR, "mmap");
    stack.limit = stack.base + getpagesize() / sizeof(qvalue);
    stack.ptr = stack.limit;
}

static void jitExec(void) {
    int error;
    error = mprotect(code.base, getpagesize(), PROT_READ | PROT_EXEC);
    if (error) err(EX_OSERR, "mprotect");
    stack.ptr = ((jitFn)code.base)(stack.ptr);
    if (stack.ptr > stack.limit) stack.ptr = stack.limit;
    error = mprotect(code.base, getpagesize(), PROT_READ | PROT_WRITE);
    if (error) err(EX_OSERR, "mprotect");
}

static void jitSrc(const char *src) {
    bool quote = false;
    while (*src) {
        if (quote) {
            jitPush(*src++);
            quote = false;
            continue;
        }

        switch (*src) {
            case ' ': break;
            case 39: quote = true; break;
            case 'B': radix = 2;   break;
            case 'O': radix = 8;   break;
            case 'D': radix = 10;  break;
            case 'X': radix = 16;  break;
            case ';': jitDop(DOP_DROP); break;
            case ':': jitDop(DOP_DUP);  break;
            case 92:  jitDop(DOP_SWAP); break;
            case '_': jitDop(DOP_NEG);  break;
            case '+': jitDop(DOP_ADD);  break;
            case '-': jitQop(QOP_SUB);  break;
            case '*': jitQop(QOP_MUL);  break;
            case '/': jitQop(QOP_DIV); jitDop(DOP_QUO); break;
            case '%': jitQop(QOP_DIV); jitDop(DOP_REM); break;
            case '~': jitDop(DOP_NOT);  break;
            case '&': jitDop(DOP_AND);  break;
            case '|': jitDop(DOP_OR);   break;
            case '^': jitDop(DOP_XOR);  break;
            case '<': jitQop(QOP_SHL);  break;
            case '>': jitQop(QOP_SHR);  break;
            case ',': jitCall(rtPrintAscii); break;
            case '.': switch (radix) {
                case 2:  jitCall(rtPrintBin); break;
                case 8:  jitCall(rtPrintOct); break;
                case 10: jitCall(rtPrintDec); break;
                case 16: jitCall(rtPrintHex); break;
            } break;

            default: {
                char *rest;
                qvalue val = strtol(src, &rest, radix);
                if (rest != src) {
                    src = rest;
                    jitPush(val);
                    continue;
                }
            }
        }

        src++;
    }
}

static char *prompt(EditLine *el __attribute((unused))) {
    static char buf[4096];
    char *bufPtr = buf;
    for (qvalue *stackPtr = stack.limit - 1; stackPtr >= stack.ptr; --stackPtr) {
        size_t bufLen = sizeof(buf) - (bufPtr - buf) - 2;
        switch (radix) {
            case 2:  bufPtr += snprintf(bufPtr, bufLen, " %s", formatBin(*stackPtr)); break;
            case 8:  bufPtr += snprintf(bufPtr, bufLen, " %lo", *stackPtr); break;
            case 10: bufPtr += snprintf(bufPtr, bufLen, " %ld", *stackPtr); break;
            case 16: bufPtr += snprintf(bufPtr, bufLen, " %lx", *stackPtr); break;
        }
    }
    buf[0] = '[';
    if (bufPtr == buf) bufPtr++;
    *bufPtr++ = ']';
    *bufPtr++ = ' ';
    *bufPtr = 0;
    return buf;
}

int main(int argc, char *argv[]) {
    jitInit();
    stackInit();

    if (argc > 1) {
        jitBegin();
        for (int i = 1; i < argc; ++i)
            jitSrc(argv[i]);
        jitEnd();
        jitExec();
        return EX_OK;
    }

    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    el_set(el, EL_PROMPT, prompt);
    el_set(el, EL_SIGNAL, true);

    for (;;) {
        int count;
        const char *line = el_gets(el, &count);
        if (count < 0) err(EX_IOERR, "el_gets");
        if (!line) break;

        jitBegin();
        jitSrc(line);
        jitEnd();
        jitExec();
    }

    el_end(el);
    return EX_OK;
}
