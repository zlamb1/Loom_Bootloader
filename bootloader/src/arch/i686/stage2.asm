BITS 16

SECTION .stage2 EXEC

GLOBAL _start2

EXTERN _print

_start2:
    mov si, msg
    call _print

.loop
    jmp .loop

msg: DB "Stage 2", 0xA, 0xD, 0