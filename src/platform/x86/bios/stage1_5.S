.code16

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1_5)

.global _start1_5
.global loom_modbase

.global loomEnterRealMode
.global loomEnterProtectedMode

#define SEGMENTS(x) mov %x, %ds; mov %x, %ss; mov %x, %es; mov %x, %fs; mov %x, %gs

_start1_5:
    PRINT(_msg)

    # Try to enable A20.
    mov $0x2401, %ax
    int $0x15
    jnc 1f
    ERROR(_a20)
1:
    call loomEnterProtectedMode
.code32
    # Relocate modules.
    movl _load, %ecx
    cmp $0, %ecx
    je 1f 
    mov $_eboot, %esi
    mov $0x100000, %edi
    mov %edi, loom_modbase
    shl $9, %ecx
    rep movsb
1:
    # Zero BSS for the C runtime.
    cld
    xor %eax, %eax
    mov $_sbss, %edi
    mov $_ebss, %ecx
    sub %edi, %ecx
    rep stosb

    # Ensure the stack is aligned.
    mov $SP, %esp

    # Jump to C!
    jmp loomMain

loomEnterRealMode:
    # Don't interrupt us while switching modes.
    cli

    # Save protected mode IDTR.
    sidt _prot_idtr

    jmp $RCODE_SEG, $_flush_real

.code16
_flush_real:
    # Load segment registers for real mode transition.
    mov $RDATA_SEG, %ax
    SEGMENTS(ax)

    lidt _real_idtr

    mov %cr0, %eax
    btr $0, %eax
    mov %eax, %cr0

    # Zero the code segment and complete the real mode transition.
    ljmp $0, $_real

_real:
    # Zero the segment registers for real mode.
    xor %ax, %ax
    SEGMENTS(ax)

    # Note: The return address must be accessible within
    # the first 65K of memory (e.g. the first segment).

    # Return to caller; now in real mode.
    # Caller was in protected mode, so return address is a long.
    pop %eax
    jmp *%ax

.code16
loomEnterProtectedMode:
    # Don't interrupt us while switching modes.
    cli

    # Save real IDTR.
    sidt _real_idtr
    
    # Load protected mode GDT.
    lgdt _gdtr
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0
    ljmp $PCODE_SEG, $_flush_prot 

.code32
_flush_prot:
    # Setup segments.
    mov $PDATA_SEG, %ax
    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Load protected IDTR.
    lidt _prot_idtr

    # Return to caller; now in protected mode.
    # Caller was in real mode, so return address is a word.
    popw %ax

    jmp *%ax

_msg:
    .asciz "Stage 1.5"

_a20:
    .asciz "Could not enable A20."

_gdt_start:
    .quad 0x0
_pcode_seg:
    .word 0xFFFF
    .word 0x0
    .byte 0x0
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x0
_pdata_seg:
    .word 0xFFFF
    .word 0x0
    .byte 0x0
    .byte 0b10010010
    .byte 0b11001111
    .byte 0x0
_rcode_seg:
    .word 0xFFFF
    .word 0x0
    .byte 0x0
    .byte 0b10011010
    .byte 0x0
    .byte 0x0
_rdata_seg:
    .word 0xFFFF
    .word 0x0
    .byte 0x0
    .byte 0b10010010
    .byte 0x0
    .byte 0x0
_gdt_end:

_real_idtr:
    .word 0
    .long 0

_prot_idtr:
    .word 0
    .long 0

_gdtr:
    .word _gdt_end - _gdt_start - 1
    .long _gdt_start

loom_modbase:
    .long 0