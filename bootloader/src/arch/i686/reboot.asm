BITS 32

SECTION .stage2.reboot EXEC

GLOBAL loom_arch_reboot

EXTERN _enter_rmode

loom_arch_reboot:
    call _enter_rmode

BITS 16
    ; request cold reboot
    mov WORD [0x472], 0
    ; jump to reset vector
    jmp 0xF000:0xFFF0