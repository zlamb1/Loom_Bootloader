BITS 16

SECTION .stage1 EXEC

GLOBAL _start
GLOBAL _print

GLOBAL _drive

GLOBAL _counter

GLOBAL _read_packet
GLOBAL _blocks
GLOBAL _transfer_offset
GLOBAL _transfer_segment
GLOBAL _start_block

EXTERN stage2s
EXTERN s2_sectors

EXTERN _start2

_start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    cld
    jmp 0:.cs

.cs:
    mov [_drive], dl
    mov si, msg
    call _print

    mov ax, stage2s
    shr ax, 4

    mov WORD [_transfer_segment], ax

    mov WORD [_counter], s2_sectors

.read:
    mov ax, [_counter]
    cmp ax, 0
    je .done

    mov ah, 0x42
    mov dl, [_drive]
    mov si, _read_packet

    int 0x13

    add DWORD [_transfer_segment], 0x20
    inc DWORD [_start_block]
    dec WORD [_counter]

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

_drive: DB 0

_counter: DW 0

_read_packet: 
    DB 0x10
    DB 0x00
_blocks:
    DW 0x01
_transfer_offset:
    DW 0x00
_transfer_segment:
    DW 0x00
_start_block:
    DQ 0x01

TIMES 510 - ($-$$) db 0
DB 0x55
DB 0xAA