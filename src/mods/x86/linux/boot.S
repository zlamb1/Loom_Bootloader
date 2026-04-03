.code32

.text

.global linuxRelocator
.global linuxRelocatorEnd

linuxRelocator:
    cli
    add $4, %esp
    mov $loomMemMove, %eax
    call *%eax
    mov $loomEnterRealMode, %eax
    call *%eax
.code16    
    mov %sp, %bp
    movw 12(%bp), %ax

    mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov $0xE000, %sp

    add $0x20, %ax
    push %ax
    push $0

    mov %sp, %bp
    add $4, %sp

    ljmp *(%bp)
linuxRelocatorEnd: