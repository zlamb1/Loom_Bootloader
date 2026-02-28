BITS 32

SECTION .stage2.isr EXEC

GLOBAL loom_isr_wrapper
GLOBAL loom_vectors

loom_isr_wrapper:
    pushad
    mov ebp, esp
    cld
    sub esp, 8
    and esp, -16

    ; stub pushes intno
    mov eax, [ebp+32]

    mov ebx, [loom_vectors + eax*4]
    cmp ebx, 0
    je .done

    mov [esp], eax
    mov eax, [ebp+36]
    mov [esp+4], eax

    call ebx

.done    
    mov esp, ebp
    popad
    ; pop intno and error code
    add esp, 8
    iret

%macro isr_stub 1
isr_stub_%1:
    %if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17 || %1 == 21 || %1 == 29 || %1 == 30)
    push 0
    %endif
    push %1
    jmp loom_isr_wrapper
%endmacro

%assign i 0
%rep 256
    isr_stub i
    %assign i i+1
%endrep

loom_vectors:
%assign i 0
%rep 256
    DD isr_stub_%+i
    %assign i i+1
%endrep