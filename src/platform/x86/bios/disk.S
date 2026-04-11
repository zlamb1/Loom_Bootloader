.code32

#define __ASM__

#include "loom/platform/x86/bios/boot.h"
#include "loom/platform/x86/bios/bios.h"

SECTION(.stage1_5)

.global biosDiskReadImpl
.global bios_io_reqs

biosDiskReadImpl:
    push %ebp # 16
    pushfl    # 12
    push %ebx #  8
    push %esi #  4
    push %edi #  0

    cli
    
    movl %esp, _stack 

    # Store count in %ecx.
    # Note: The count must fit in a word register and be <= BIOS_IO_REQS_LIMIT.
    mov 24(%esp), %ecx

    call loomEnterRealMode
.code16
    push $bios_io_reqs
    push %cx
    movw %sp, %bp
1:
    movw $0, %bx
    movw (%bp), %cx
    cmp $0, %cx
    je 4f

    movw 2(%bp), %bx
    movb $0x10, _dap_size

    movw 8(%bx), %cx
    movw %cx, _dap_count
    movw 10(%bx), %cx
    movw %cx, _dap_off
    movw 12(%bx), %cx
    movw %cx, _dap_seg
    movl (%bx), %ecx
    movl %ecx, _dap_block
    movl 4(%bx), %ecx
    movl %ecx, _dap_block+4

    mov $0x42, %ah
    movb 14(%bx), %dl
    mov $_dap, %si
    int $0x13
    jc 2f
    jmp 3f
2:
    mov $-1, %bx
    jmp 4f
3:
    decw (%bp)
    addw $16, 2(%bp)
    jmp 1b
4:
    call loomEnterProtectedMode
.code32
    movl _stack, %esp 

    movzx %bx, %eax

    pop %edi
    pop %esi
    pop %ebx
    popfl
    pop %ebp
    ret

bios_io_reqs:
    .rept BIOS_IO_REQS_LIMIT
    .quad 0
    .quad 0
    .endr