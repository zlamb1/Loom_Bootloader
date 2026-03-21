.code32

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1_5)

.global loomIsrWrapper
.global loom_vectors

loomIsrWrapper:
    pushal
    mov %esp, %ebp
    cld
    sub $8, %esp
    and $-16, %esp

    movl 32(%ebp), %eax

    movl loom_vectors(,%eax,4), %ebx
    cmp $0, %ebx
    je 1f

    mov %eax, (%esp)
    mov 36(%ebp), %eax
    mov %eax, 4(%esp)

    call *%ebx

1:
    mov %ebp, %esp
    popal
    add $8, %esp
    iret

.rept 256
isr_stub_\+:
    .if !(\+ == 8 || (\+ >= 10 && \+ <= 14) || \+ == 17 || \+ == 21 || \+ == 29 || \+ == 30)
        pushl 0
    .endif
    pushl $\+
    jmp loomIsrWrapper
.endr

loom_vectors:
.rept 256
    .long isr_stub_\+
.endr