#if 0
exec cc -Wall -Wextra $@ -o $(dirname $0)/jrp $0
#endif

#include <err.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <unistd.h>

typedef unsigned long long op;
typedef long long value;
typedef unsigned long long uvalue;
typedef value *(*fptr)(value *);

enum {
    HOP_NOP  = 0x90666666, // nop
    HOP_RET  = 0x906666c3, // ret
    HOP_DROP = 0x9066665f, // pop rdi
    HOP_DUP  = 0x90666657, // push rdi
    HOP_SWAP = 0x244c8748, // xchg rdi, [rsp]
    HOP_NEG  = 0x90dff748, // neg rdi
    HOP_ADD  = 0xc7014858, // pop rax; add rdi, rax
    HOP_QUO  = 0x90c78948, // mov rdi, rax
    HOP_REM  = 0x90d78948, // mov rdi, rdx
    HOP_NOT  = 0x90d7f748, // not rdi
    HOP_AND  = 0xc7214858, // pop rax; and rdi, rax
    HOP_OR   = 0xc7904858, // pop rax; or rdi, rax
    HOP_XOR  = 0xc7314858, // pop rax; xor rdi, rax
};

enum {
    FOP_PROL = 0x5ffc8948e5894855, // push rbp; mov rbp, rsp; mov rsp, rdi; pop rdi
    FOP_EPIL = 0x5dec8948e0894857, // push rdi; mov rax, rsp; mov rsp, rbp; pop rbp
    FOP_CRT  = 0xb848906666e58748, // xchg rsp, rbp; mov rax, strict qword 0
    FOP_CALL = 0x90665fe58748d0ff, // call rax; xchg rsp, rbp; pop rdi
    FOP_PUSH = 0xbf48909066666657, // push rdi; mov rdi, strict qword 0
    FOP_SUB  = 0x9066665f243c2948, // sub [rsp], rdi; pop rdi
    FOP_MUL  = 0x906666f8af0f4858, // pop rax; imul rdi, rax
    FOP_DIV  = 0x9066fff748994858, // pop rax; cqo; idiv rdi
    FOP_SHL  = 0x5f2424d348f98948, // mov rcx, rdi; shl qword [rsp], cl; pop rdi
    FOP_SHR  = 0x5f242cd348f98948, // mov rcx, rdi; shr qword [rsp], cl; pop rdi
};

#define FOP(a, b) ((op)(b) << 32 | (op)(a))

#define JIT_PUSH(p, x) { \
    *(p)++ = FOP_PUSH; \
    *(p)++ = x; \
}

#define JIT_CALL(p, fn) { \
    *(p)++ = FOP_CRT; \
    *(p)++ = (op)(fn); \
    *(p)++ = FOP_CALL; \
}

static void rt_print_ascii(value val) {
    printf("%c\n", (char)val);
}

static void rt_print_bin(value val) {
    static char buf[sizeof(value) * 8 + 1];
    uvalue uval = val;
    int i = sizeof(buf);
    do {
        buf[--i] = '0' + (uval & 1);
    } while (uval >>= 1);
    printf("%s\n", &buf[i]);
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

int main() {
    int error;
    int page = getpagesize();

    value *base = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (base == MAP_FAILED) err(EX_OSERR, "mmap");
    value *stack = base + page / sizeof(value) - 1;

    op *ops = mmap(0, page, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (ops == MAP_FAILED) err(EX_OSERR, "mmap");

    op *p = ops;
    *p++ = FOP_PROL;
    JIT_PUSH(p, 7);
    JIT_PUSH(p, 10);
    JIT_PUSH(p, 9);
    *p++ = FOP_MUL;
    *p++ = FOP(HOP_ADD, HOP_DUP);
    *p++ = FOP(HOP_DUP, HOP_DUP);
    *p++ = FOP(HOP_DUP, HOP_NOP);
    JIT_CALL(p, rt_print_ascii);
    JIT_CALL(p, rt_print_bin);
    JIT_CALL(p, rt_print_oct);
    JIT_CALL(p, rt_print_dec);
    JIT_CALL(p, rt_print_hex);
    *p++ = FOP_EPIL;
    *p++ = FOP(HOP_RET, HOP_NOP);

    error = mprotect(ops, page, PROT_READ | PROT_EXEC);
    if (error) err(EX_OSERR, "mprotect");

    fptr fn = (fptr)ops;
    stack = fn(stack);

    printf("%lld\n", *stack);

    return 0;
}
