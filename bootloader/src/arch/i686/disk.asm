BITS 16

SECTION .stage2.disk EXEC

GLOBAL _load_stage3
GLOBAL _load_mods

EXTERN _print

EXTERN _drive

EXTERN _modsize

EXTERN stage1s
EXTERN stage3s
EXTERN stage3e

EXTERN s2_sectors
EXTERN s3_sectors

ALIGN 4

_dap:
    .size:    DB 0
    .reserved DB 0
    .count:   DW 0
    .off:     DW 0
    .seg:     DW 0
    .block:   DQ 0

;
; eax: block count
; ebx: flat buffer address
; ecx: first block
;
; preconditions:
; ebx + eax*sector_size < 1 MiB
;

_load_sectors:
    push eax
    push ebx
    push ecx

    mov bp, sp

.read:
    mov eax, DWORD [bp+8]
    cmp eax, 0
    je .done

    mov eax, DWORD [bp+4]
    mov dx, ax
    and dx, 15
    shr eax, 4

    mov ecx, DWORD [bp]

    mov BYTE [_dap.size], 0x10
    mov BYTE [_dap.reserved], 0
    mov WORD [_dap.count], 1
    mov WORD [_dap.off], dx
    mov WORD [_dap.seg], ax
    mov DWORD [_dap.block], ecx
    mov DWORD [_dap.block+4], 0

    mov ah, 0x42
    mov dl, BYTE [_drive]
    mov si, _dap

    ; guard against buggy BIOSes overwriting bp
    ; if they overwrite sp, we can't do anything

    push bp
    clc
    int 0x13
    pop bp

    jc .fail
    mov ax, WORD [_dap.count]
    cmp ax, 1
    jne .fail

    dec DWORD [bp+8]
    add DWORD [bp+4], 512
    inc DWORD [bp]

    jmp .read

.done:
    add sp, 12

    ret

.fail:
    mov si, disk_fail
    call _print
    cli
    hlt

_load_stage3:
    mov eax, s3_sectors
    mov ebx, stage3s

    mov ecx, stage3s
    sub ecx, stage1s 
    shr ecx, 9
    
    call _load_sectors

    ret

_load_mods:
    mov eax, 1
    mov ebx, stage3e

    mov ecx, stage3e
    sub ecx, stage1s
    shr ecx, 9

    push ecx

    call _load_sectors

    ; verify magic
    mov eax, DWORD [stage3e]
    cmp eax, 0x70D61E9C
    je .next
    mov si, magic_fail
    jmp .fail

.next
    ; get module header size
    mov eax, DWORD [stage3e+12]

    ; align up to sector size
    add eax, 511
    and eax, -512

    ; store size
    mov DWORD [_modsize], eax

    ; compute block count
    shr eax, 9
    mov ebx, stage3e
    pop ecx

    ; load modules into memory after stage3e
    call _load_sectors

    ret

.fail:
    mov si, magic_fail
    call _print
    cli
    hlt

disk_fail: DB "Disk Read Fail", 0xA, 0xD, 0
magic_fail: DB "Bad Module Header Magic", 0xA, 0xD, 0