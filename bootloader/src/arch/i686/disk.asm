BITS 16

SECTION .stage2.disk EXEC

GLOBAL _load_stage3

EXTERN _print

EXTERN _drive

EXTERN stage3s

EXTERN s2_sectors
EXTERN s3_sectors

_load_stage3:
    push bp
    mov bp, sp

    sub sp, 20

    ; align read packet to 4 bytes
    and sp, -4

    ; use bx since sp cannot be used as a base
    mov bx, sp

    ; compute offset and segment of buffer start
    mov ax, stage3s
    mov cx, 16
    mov dx, 0
    div cx

    mov BYTE [bx], 16
    mov BYTE [bx+1], 0
    mov WORD [bx+2], 1
    mov WORD [bx+4], dx
    mov WORD [bx+6], ax

    mov eax, s2_sectors
    ; include the boot sector
    inc eax

    mov DWORD [bx+8], eax
    mov DWORD [bx+12], 0
    mov DWORD [bx+16], s3_sectors

.read:
    mov eax, DWORD [bx+16]
    cmp eax, 0
    je .done
    
    mov ah, 0x42
    mov dl, BYTE [_drive]
    mov si, sp

    int 0x13

    ; restore bx in case BIOS clobbered it
    mov bx, sp

    jc .fail
    cmp ah, 0
    jne .fail
    mov ax, [bx+2]
    cmp ax, 1
    jne .fail
    
    add WORD [bx+6], 0x20
    add DWORD [bx+8], 1
    adc DWORD [bx+12], 0
    dec DWORD [bx+16]

    jmp .read

.done:
    mov sp, bp
    pop bp
    ret

.fail:
    mov si, disk_fail
    call _print

.loop:
    jmp .loop

disk_fail: DB "Disk Read Fail", 0xA, 0xD, 0