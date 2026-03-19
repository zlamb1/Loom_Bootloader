BITS 32

SECTION .text

GLOBAL loomMemCopy
GLOBAL loomMemset

EXTERN loomPanic

s1: DB "loomMemCopy", 0
s2: DB "loomMemSet", 0

loomMemCopy:
    push esi
    push edi
    mov edi, [esp+12]
    cmp edi, 0
    jne .1
    push s1
    call loomPanic
.1:
    mov esi, [esp+16]
    cmp esi, 0
    jne .2
    push s1
    call loomPanic
.2:
    mov ecx, [esp+20]
    rep movsb
    pop edi
    pop esi
    ret

loomMemSet:
    push edi
    mov edi, [esp+8]
    cmp edi, 0
    jne .1
    push s2
    call loomPanic
.1:
    mov eax, [esp+12]
    mov ecx, [esp+16]
    rep stosb
    pop edi
    ret