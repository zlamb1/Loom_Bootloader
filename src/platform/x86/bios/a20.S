.code16

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1_5)

.global loomEnableA20
.global loomDisableA20
.global loomQueryA20

#define QUERY() call loomQueryA20; cmp $1, %ax; je .edone

loomEnableA20:
    # Don't do anything if A20 is already enabled.
    QUERY()

    # Try BIOS first.
    mov $0x2401, %ax
    int $0x15
    QUERY()

    # Try Fast A20.
    in $0x92, %al
    or $2, %al
    out %al, $0x92
    QUERY()

    # Try 0xee.
    in $0xee, %al
    QUERY()

    ERROR(_a20e)
.edone:
    ret

#undef QUERY
#define QUERY() call loomQueryA20; cmp $0, %ax; je .ddone

loomDisableA20:
    # Don't do anything if A20 is already disabled.
    QUERY()

    # Try BIOS first.
    mov $0x2400, %ax
    int $0x15
    QUERY()

    # Try 0xee.
    out %al, $0xee
    QUERY()

    ERROR(_a20d)
.ddone:
    ret

loomQueryA20:
    pushf
    push %ds
    push %es

    cli

    # Setup the segments for the wrap test.
    xor %ax, %ax
    mov %ax, %ds
    mov $0xFFFF, %ax
    mov %ax, %es

    # Save whatever is in the memory we use for the test.
    movw %ds:(0x7cfe), %ax
    push %ax

    movw $0xAA55, %ds:(0x7cfe)
    # Note that we have to add 0x10 to 0x7cfe to get the correct
    # address since %es is one less than 0x10000.
    movw %es:(0x7d0e), %ax
    cmp $0xAA55, %ax
    jne 1f

    # Perform the test again with a different value to confirm
    # the results.
    movw $0x55AA, %ds:(0x7cfe)
    movw %es:(0x7d0e), %ax
    cmp $0x55AA, %ax
    jne 1f

    mov $0, %ax
    jmp 2f
1:
    mov $1, %ax
2:
    # Restore the memory we clobbered for the test.
    pop %es
    movw %es, %ds:(0x7cfe)

    pop %es
    pop %ds
    popf

    ret

_a20e:
    .asciz "Could not enable the A20 line."

_a20d:
    .asciz "Could not disable the A20 line."