BITS 32

SECTION .stage2.int EXEC

GLOBAL loom_bios_int

EXTERN _enter_rmode
EXTERN _enter_pmode

_stack: DD 0

; This should not be called after reconfiguring any hardware
; like the PIC, PIT, etc...

loom_bios_int:
    pushfd
    push ebx
    push esi
    push edi
    push ebp

    mov DWORD [_stack], esp

    mov eax, DWORD [esp+28]

    mov ebx, DWORD [eax]
    mov DWORD [_eax], ebx

    mov bx, WORD [eax+24]
    mov WORD [_flags], bx

    mov bx, WORD [eax+26]
    mov WORD [_ds], bx

    mov bx, WORD [eax+28]
    mov WORD [_es], bx

    mov bl, BYTE [esp+24]
    mov BYTE [_intno], bl

    mov ebx, DWORD [eax+4]
    mov ecx, DWORD [eax+8]
    mov edx, DWORD [eax+12]
    mov esi, DWORD [eax+16]
    mov edi, DWORD [eax+20]

    call _enter_rmode

BITS 16
            DB 0xB8
    _ds:    DW 0

    mov ds, ax

            DB 0xB8
    _es:    DW 0

    mov es, ax

            DB 0xB8
    _flags: DW 0

    push ax
    popf

            DB 0x66, 0xB8
    _eax:   DD 0

            DB 0xCD
    _intno: DB 0

    mov DWORD cs:[_eax], eax
    
    pushf
    pop ax
    mov WORD cs:[_flags], ax

    mov WORD cs:[_ds], ds
    mov WORD cs:[_es], es
    
    xor ax, ax
    mov ds, ax
    mov es, ax

    call _enter_pmode

BITS 32
    mov esp, [_stack]

    mov eax, DWORD [esp+28]
    
    mov DWORD [eax+4], ebx
    mov DWORD [eax+8], ecx
    mov DWORD [eax+12], edx
    mov DWORD [eax+16], esi
    mov DWORD [eax+20], edi

    mov ebx, DWORD [_eax]
    mov DWORD [eax], ebx

    mov bx, WORD [_flags]
    mov WORD [eax+24], bx

    mov bx, WORD [_ds]
    mov WORD [eax+26], bx

    mov bx, WORD [_es]
    mov WORD [eax+28], bx

    pop ebp
    pop edi
    pop esi
    pop ebx
    popf

    ret