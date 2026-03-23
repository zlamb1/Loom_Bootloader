.code16

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1)

.global _start
.global _drive
.global _error
.global _load
.global _puts

_start:
    # Avoid FAT BPB in case we are installed in VBR.
    jmp _setup
    nop

.fill 90 - (. - _start), 1, 0

_setup:
    # Don't interrupt us.
    cli

    # Setup segment registers.
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %ss

    # Setup stack.
    mov $SP, %sp

    # Save drive number.
    mov %dl, _drive

    sti
    
    # Greetings.
    PRINT(_msg)

    # Check for int 13h,AH=42h support.
    mov $0x41, %ah
    mov $0x55AA, %bx
    mov _drive, %dl
    int $0x13
    jc 1f
    cmp $0xAA55, %bx
    jne 1f
    jmp 2f
1:
    ERROR(_unsupported_disk)
2:
    mov $_sstage1_5, %ax
    # Stage 1.5 must be sector aligned.
    shr $4, %ax

    movw $0, _dap_off
    mov %ax, _dap_seg

    mov $_sectors, %eax
    # The boot sector is already loaded.
    dec %eax
    # Note: _load is used as a field that is
    # dynamically modified by image creation
    # utilities with the purpose of appending
    # core modules.
    addl _load, %eax
1:
    # Use the stack to track blocks remaining across loads.
    push %eax
    cmp $0, %eax
    je 3f

    # Load the sectors of stage 1.5 and 2.
    mov $0x42, %ah
    # Buggy BIOSes may clobber %dl?
    mov _drive, %dl
    mov $_dap, %si
    # Load individual sectors to avoid crossing 64K boundaries.
    int $0x13
    jnc 2f
    ERROR(_disk_error)
2:
    addw $0x20, _dap_seg
    addl $1, _dap_block
    adcl $0, _dap_block+4
    pop %eax
    dec %eax
    jmp 1b
3:
    # Go to Stage 1.5 and zero code segment.
    ljmp $0, $_start1_5

_putchar:
    mov $0xE, %ah
    mov $0, %bh
    int $0x10
    ret

_puts:
    cld
    lodsb
    cmp $0, %al
    je 2f
    call _putchar
    jmp _puts
2:
    mov $'\r', %al
    call _putchar
    mov $'\n', %al
    call _putchar
    ret

_error:
    call _puts
    cli
    hlt

_msg:
    .asciz "Stage 1"

_unsupported_disk:
    .asciz "Disk not supported."

_disk_error:
    .asciz "Disk read error."

_drive:
    .byte 0x0

.fill 400 - (. - _start), 1, 0

_dap:
_dap_size:
    .byte 0x10
    .byte 0x0
_dap_count:
    .word 0x1
_dap_off:
    .word 0x0
_dap_seg:
    .word 0x0
_dap_block:
    .quad 0x1

_load:
    .long 0

.fill 510 - (. - _start), 1, 0

.byte 0x55
.byte 0xAA