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
typedef value *(*fptr)(value *);

enum {
    OP_PROL = 0x90fc8948e5894855, // push ebp; mov rbp, rsp; mov rsp, rdi
    OP_EPIL = 0xc35dec8948e08948, // mov rax, rsp; mov rsp, rbp; pop rbp; ret
    OP_CALL = 0x90666666d0ff5f58, // pop rax; pop rdi; call rax
    OP_PUSH = 0x9066660000000068, // push strict dword 0
    OP_DROP = 0x9066666608c48348, // add rsp, 8
    OP_DUP  = 0x90906666662434ff, // push qword [rsp]
    OP_SWAP = 0x9066666650515859, // pop rcx; pop rax; push rcx; push rax
    OP_NEG  = 0x90666666241cf748, // neg qword [rsp]
    OP_ADD  = 0x906666240c014859, // pop rcx; add [rsp], rcx
    OP_SUB  = 0x906666240c294859, // pop rcx; sub [rsp], rcx
    OP_MUL  = 0x9050c1af0f485859, // pop rcx; pop rax; imul rax, rcx; push rax
    OP_DIV  = 0x50f9f74899485859, // pop rcx; pop rax; cqo; idiv rcx; push rax
    OP_REM  = 0x52f9f74899485859, // pop rcx; pop rax; cqo; idiv rcx; push rdx
    OP_NOT  = 0x906666662414f748, // not qword [rsp]
    OP_AND  = 0x906666240c214859, // pop rcx; and [rsp], rcx
    OP_OR   = 0x906666240c904859, // pop rcx; or [rsp], rcx
    OP_XOR  = 0x906666240c314859, // pop rcx; xor [rsp], rcx
    OP_SHL  = 0x9066662424d34859, // pop rcx; shl qword [rsp], cl
    OP_SHR  = 0x906666242cd34859, // pop rcx; shr qword [rsp], cl
};

int main() {
    int error;
    int page = getpagesize();

    value *base = mmap(0, page, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (base == MAP_FAILED) err(EX_OSERR, "mmap");
    value *stack = base + page / sizeof(value);

    op *ops = mmap(0, page, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (ops == MAP_FAILED) err(EX_OSERR, "mmap");

    op *p = ops;
    *p++ = OP_PROL;
    *p++ = OP_PUSH | (1 << 8);
    *p++ = OP_PUSH | (2 << 8);
    *p++ = OP_ADD;
    *p++ = OP_DUP;
    *p++ = OP_MUL;
    *p++ = OP_EPIL;

    error = mprotect(ops, page, PROT_READ | PROT_EXEC);
    if (error) err(EX_OSERR, "mprotect");

    fptr fn = (fptr) ops;
    stack = fn(stack);

    printf("%lld\n", *stack);

    return 0;
}
