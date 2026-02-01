BITS 32

SECTION .text

GLOBAL loom_memcpy
GLOBAL loom_memset

loom_memcpy:
    push esi
    push edi
    mov edi, [esp+12]
    mov esi, [esp+16]
    mov ecx, [esp+20]
    rep movsb
    pop edi
    pop esi
    ret

loom_memset:
    push edi
    mov edi, [esp+8]
    mov eax, [esp+12]
    mov ecx, [esp+16]
    rep stosb
    pop edi
    ret