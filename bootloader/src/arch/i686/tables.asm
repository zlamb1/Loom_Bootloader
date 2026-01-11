BITS 16

SECTION .stage2.tables EXEC

GLOBAL _save_real_idt
GLOBAL _enter_pmode

_real_idtr:
    DW 0
    DD 0

_gdt_start:
    DQ 0x00000000
_prot_code_seg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10011010
    DB 0b11001111
    DB 0x00000000
_prot_data_seg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10010010
    DB 0b11001111
    DB 0x00000000
_real_code_seg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10011010
    DB 0b00000000
    DB 0x00000000
_real_data_seg:
    DW 0x0000FFFF
    DW 0x00000000
    DB 0x00000000
    DB 0b10010010
    DB 0b00000000
    DB 0x00000000
_gdt_end:

_gdtr:
    DW _gdt_end - _gdt_start - 1
    DD _gdt_start

_save_real_idt:
    ; store the real mode IDT for use on return to real mode
    sidt [_real_idtr]
    ret

BITS 32

_enter_rmode:
    cli
    jmp 0x18:.flush

BITS 16

.flush
    mov ax, 0x20
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    lidt [_real_idtr]

    ; Caller must have paging already disabled.
    mov eax, cr0
    btr eax, 0
    mov cr0, eax

    jmp 0:.real

.real
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    sti

    ; We assume the caller has a valid stack pointer
    ; for real mode.

    ; The return address is 4 bytes.
    ; It must be addressable in real mode.

    pop eax
    jmp ax

_enter_pmode:
    cli
    lgdt [_gdtr]
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