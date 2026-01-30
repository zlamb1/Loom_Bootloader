BITS 32

SECTION .text

GLOBAL linux_relocator
GLOBAL linux_relocator_end

EXTERN loom_enter_rmode

linux_relocator:
    cli
    call loom_enter_rmode
BITS 16
    mov bp, sp
    mov eax, [bp+4]

    sub sp, 8
    mov bp, sp

    mov WORD [bp], 0
    mov WORD [bp+4], ax
    add WORD [bp+4], 0x20

    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov sp, 0xE000

    jmp far [cs:bp]
linux_relocator_end: