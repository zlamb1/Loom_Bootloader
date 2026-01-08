BITS 16

SECTION .stage2.tables EXEC

GLOBAL _enter_pmode

_GDT:
    DQ 0x00000000
_PCodeSeg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10011001
    DB 0b01001111
    DB 0x00000000
_PDataSeg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10010011
    DB 0b01001111
    DB 0x00000000
_GDTEnd:

_GDTR:
    DW _GDTEnd - _GDT - 1
    DD _GDT

_enter_pmode:
    cli
    lgdt [_GDTR]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x8:.flush

BITS 32

.flush:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; the return address is 2 bytes
    pop ax
    movzx eax, ax

    jmp eax