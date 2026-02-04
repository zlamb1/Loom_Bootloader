set(LOOM_ARCH x86)

set(LOOM_ARCH_HDR_DIR "${LOOM_INCLUDE_DIR}/loom/arch/i686")
set(LOOM_ARCH_SRC_DIR "${LOOM_SRC_DIR}/arch/i686")
set(LOOM_LINKER_SCRIPT "${LOOM_ARCH_SRC_DIR}/loom.ld")

set(LOOM_MOD_ARCH_SRC_DIR "${LOOM_MOD_SRC_DIR}/x86")

set(RUN_QEMU "qemu-system-x86_64")
set(DBG_QEMU "qemu-system-i386")

list(APPEND LOOM_INCLUDE_DIRS "${LOOM_ARCH_HDR_DIR}/std")

list(
    APPEND
    LOOM_HDRS
    "${LOOM_ARCH_HDR_DIR}/bios.h"
    "${LOOM_ARCH_HDR_DIR}/idt.h"
    "${LOOM_ARCH_HDR_DIR}/io.h"
    "${LOOM_ARCH_HDR_DIR}/isr.h"
    "${LOOM_ARCH_HDR_DIR}/pic.h"
)

list(
    APPEND 
    LOOM_SRCS
    "${LOOM_ARCH_SRC_DIR}/a20.asm"
    "${LOOM_ARCH_SRC_DIR}/arch.c"
    "${LOOM_ARCH_SRC_DIR}/bios.c"
    "${LOOM_ARCH_SRC_DIR}/disk.asm"
    "${LOOM_ARCH_SRC_DIR}/exception.c"
    "${LOOM_ARCH_SRC_DIR}/idt.c"
    "${LOOM_ARCH_SRC_DIR}/int.asm"
    "${LOOM_ARCH_SRC_DIR}/io.c"
    "${LOOM_ARCH_SRC_DIR}/isr.asm"
    "${LOOM_ARCH_SRC_DIR}/mem.asm"
    "${LOOM_ARCH_SRC_DIR}/pic.c"
    "${LOOM_ARCH_SRC_DIR}/reboot.asm"
    "${LOOM_ARCH_SRC_DIR}/rel.c"
    "${LOOM_ARCH_SRC_DIR}/stage1.asm"
    "${LOOM_ARCH_SRC_DIR}/stage2.asm"
    "${LOOM_ARCH_SRC_DIR}/tables.asm"
    "${LOOM_ARCH_SRC_DIR}/vga.c"
)

create_module(linux ${LOOM_MOD_ARCH_SRC_DIR}/linux/boot.asm ${LOOM_MOD_ARCH_SRC_DIR}/linux/linux.c)
create_module(ps2 ${LOOM_MOD_ARCH_SRC_DIR}/ps2.c)