BITS 16

SECTION .stage1 EXEC

GLOBAL _start
GLOBAL _print

EXTERN stage2s
EXTERN s2_sectors

EXTERN _start2

_start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    mov ss, ax
    mov sp, 0x7C00
    jmp 0:.cs

.cs:
    mov [drive], dl
    mov si, msg
    call _print

    mov ax, stage2s
    shr ax, 4

    mov WORD [transfer_segment], ax

    mov WORD [counter], s2_sectors

.read:
    mov ax, [counter]
    cmp ax, 0
    je .done

    mov ah, 0x42
    mov dl, [drive]
    mov si, read_packet

    int 0x13

    add DWORD [transfer_segment], 0x20
    inc DWORD [start_block]
    dec WORD [counter]

    jmp .read

.done
    jmp _start2

_print:
    lodsb 
    cmp al, 0
    je .done
    mov ah, 0xE
    mov bh, 0
    int 0x10
    jmp _print
.done:
    ret

msg: DB "Stage 1", 0xA, 0xD, 0

drive: DB 0

counter: DW 0

read_packet: 
    DB 0x10
    DB 0x00
    DW 0x01
transfer_offset:
    DW 0x00
transfer_segment:
    DW 0x00
start_block:
    DQ 0x01

TIMES 510 - ($-$$) db 0
DB 0x55
DB 0xAA