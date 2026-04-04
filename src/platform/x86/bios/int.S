.code32

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1_5)

.global loomBiosIntImpl

_stack:
    .long 0

#
#          Stack Frame
#
# ESP --- EDI               [00]
#     --- ESI               [04]
#     --- EBX               [08]
#     --- EFLAGS            [12]
#     --- EBP               [16]
#     --- Return Address    [20]
#     --- Interrupt Number  [24]
#     --- Registers         [28]
#

loomBiosIntImpl:
    push %ebp

    pushfl
    push %ebx
    push %esi
    push %edi

    cli

    movl %esp, _stack

    movl 28(%esp), %eax

    movl (%eax), %ebx
    movl %ebx, _eax

    movw 24(%eax), %bx
    movw %bx, _flags

    movw 26(%eax), %bx
    movw %bx, _ds

    movw 28(%eax), %bx
    movw %bx, _es

    movb 24(%esp), %bl
    movb %bl, _intno

    mov 4(%eax), %ebx
    mov 8(%eax), %ecx
    mov 12(%eax), %edx
    mov 16(%eax), %esi
    mov 20(%eax), %edi

    call loomEnterRealMode

.code16
         .byte 0xB8
    _ds: .word 0

    mov %ax, %ds

         .byte 0xB8
    _es: .word 0

    mov %ax, %es

            .byte 0xB8
    _flags: .word 0

    push %ax
    popf

          .byte 0x66, 0xB8
    _eax: .long 0

            .byte 0xCD
    _intno: .byte 0

    movl %eax, %cs:(_eax)

    pushf
    pop %ax
    movw %ax, %cs:(_flags)

    movw %ds, %cs:(_ds)
    movw %es, %cs:(_es)

    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es

    call loomEnterProtectedMode

.code32
    movl _stack, %esp
    mov %esp, %ebp

    movl 28(%esp), %eax

    mov %ebx, 4(%eax)
    mov %ecx, 8(%eax)
    mov %edx, 12(%eax)
    mov %esi, 16(%eax)
    mov %edi, 20(%eax)

    movl _eax, %ebx
    mov %ebx, (%eax)

    movw _flags, %bx
    movw %bx, 24(%eax)

    movw _ds, %bx
    movw %bx, 26(%eax)

    movw _es, %bx
    movw %bx, 28(%eax)

    pop %edi
    pop %esi
    pop %ebx
    popf

    pop %ebp
    ret