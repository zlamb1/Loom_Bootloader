BITS 16

SECTION .stage2 EXEC

GLOBAL _start2
GLOBAL _modsize

EXTERN _print
EXTERN _load_stage3
EXTERN _load_mods
EXTERN _enable_a20
EXTERN _save_real_idt
EXTERN _enter_pmode

_start2:
    mov si, msg
    call _print
    call _load_stage3
    call _load_mods
    call _enable_a20
    call _save_real_idt
    call _enter_pmode

EXTERN sbss
EXTERN ebss
EXTERN stage3e

EXTERN loom_modbase
EXTERN loom_main

BITS 32
    cld

    mov DWORD [loom_modbase], 0x100000

    ; relocate modules to high memory
    mov ecx, _modsize
    mov esi, stage3e
    mov edi, DWORD [loom_modbase]
    rep movsb

    ; zero BSS
    mov al, 0
    mov edi, sbss
    mov ecx, ebss
    sub ecx, edi
    rep stosb

    ; reset stack
    mov esp, 0x7C00

    jmp loom_main

.loop:
    jmp .loop

_modsize:     DD 0
loom_modbase: DD 0

msg: DB "Stage 2", 0xA, 0xD, 0
