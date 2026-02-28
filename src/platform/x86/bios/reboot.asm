BITS 32

SECTION .stage2.reboot EXEC

GLOBAL loom_reboot

EXTERN loom_enter_rmode

loom_reboot:
    call loom_enter_rmode

BITS 16
    ; request cold reboot
    mov WORD [0x472], 0
    ; jump to reset vector
    jmp 0xF000:0xFFF0