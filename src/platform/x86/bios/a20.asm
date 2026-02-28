BITS 16

SECTION .stage2.asm EXEC

GLOBAL _query_a20
GLOBAL _enable_a20

EXTERN _print

_query_a20:
    mov WORD [0x7DFE], 0xAA55

    mov ax, 0xFFFF
    mov es, ax

    mov ax, WORD es:[0x7E0E]
    cmp ax, 0xAA55
    jne .enabled

    mov WORD [0x7DFE], 0x55AA
    mov ax, WORD es:[0x7E0E]
    cmp ax, 0x55AA
    jne .enabled

    mov ax, 0
    jmp .done
.enabled:
    mov ax, 1
.done:
    mov WORD [0x7DFE], 0xAA55
    ret

_enable_a20:
    call _query_a20
    cmp ax, 1
    je .done

    ; try BIOS
    mov ax, 0x2403
    int 0x15
    jc .next
    cmp ah, 0
    jne .next

    mov ax, 0x2401
    int 0x15
    jc .next
    cmp ah, 0
    jne .next

    call _query_a20
    cmp ax, 1
    jne .next

    jmp .done

.next:
    ; fast A20
    in al, 0x92
    or al, 2
    out 0x92, al

    call _query_a20
    cmp ax, 1
    je .done

    jmp .fail

.done:
    ret

.fail:
    mov si, a20_fail
    call _print
    jmp .loop

.loop:
    jmp .loop

a20_fail: DB "A20 Error", 0xA, 0xD, 0