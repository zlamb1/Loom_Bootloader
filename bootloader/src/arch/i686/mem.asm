BITS 32

SECTION .text

GLOBAL loom_memcpy
GLOBAL loom_memset

EXTERN loom_panic

s1: DB "loom_memcpy", 0
s2: DB "loom_memset", 0

loom_memcpy:
    push esi
    push edi
    mov edi, [esp+12]
    cmp edi, 0
    jne .1
    push s1
    call loom_panic
.1:
    mov esi, [esp+16]
    cmp esi, 0
    jne .2
    push s1
    call loom_panic
.2:
    mov ecx, [esp+20]
    rep movsb
    pop edi
    pop esi
    ret

loom_memset:
    push edi
    mov edi, [esp+8]
    cmp edi, 0
    jne .1
    push s2
    call loom_panic
.1:
    mov eax, [esp+12]
    mov ecx, [esp+16]
    rep stosb
    pop edi
    ret