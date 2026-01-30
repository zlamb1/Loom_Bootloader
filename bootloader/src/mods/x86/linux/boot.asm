BITS 32

SECTION .text

GLOBAL linux_relocator
GLOBAL linux_relocator_end

EXTERN _enter_rmode

linux_relocator:
    cli
    call _enter_rmode
    mov eax, [esp+4]
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov sp, 0xE000
    hlt
linux_relocator_end: