.code32

#include "loom/platform/x86/bios/boot.h"

SECTION(.stage1_5)

.global loomReboot

loomReboot:
    call loomEnterRealMode
.code16
    # Request a cold reboot.
    movw $0, 0x472
    # Jump to the reset vector.
    ljmp $0xF000, $0xFFF0
