BITS 32

SECTION .text

GLOBAL linux_relocator
GLOBAL linux_relocator_end

EXTERN loom_enter_rmode
EXTERN loom_memmove

linux_relocator:
    cli
    add esp, 4
    mov eax, loom_memmove
    call eax
    mov eax, loom_enter_rmode
    call eax

BITS 16
    mov bp, sp
    mov ax, WORD [bp+12]

    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov sp, 0xE000

    add ax, 0x20
    push ax
    push 0

    mov bp, sp
    add sp, 4

    jmp far [bp]
linux_relocator_end: