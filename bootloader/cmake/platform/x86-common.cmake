set(LOOM_ARCH_HDR_DIR "${LOOM_INCLUDE_DIR}/loom/platform/x86")
set(LOOM_ARCH_SRC_DIR "${LOOM_SRC_DIR}/platform/x86")

set(LOOM_MOD_ARCH_SRC_DIR "${LOOM_MOD_SRC_DIR}/x86")

set(RUN_QEMU "qemu-system-x86_64")
set(DBG_QEMU "qemu-system-i386")

list(APPEND LOOM_INCLUDE_DIRS "${LOOM_ARCH_HDR_DIR}/std")
message(STATUS "${LOOM_INCLUDE_DIRS}")

list(
    APPEND
    LOOM_HDRS
    "${LOOM_ARCH_HDR_DIR}/idt.h"
    "${LOOM_ARCH_HDR_DIR}/io.h"
    "${LOOM_ARCH_HDR_DIR}/isr.h"
    "${LOOM_ARCH_HDR_DIR}/pic.h"
)

list(
    APPEND 
    LOOM_SRCS
    "${LOOM_ARCH_SRC_DIR}/idt.c"
    "${LOOM_ARCH_SRC_DIR}/io.c"
    "${LOOM_ARCH_SRC_DIR}/pic.c"
    "${LOOM_ARCH_SRC_DIR}/rel.c"
)

create_module(linux ${LOOM_MOD_ARCH_SRC_DIR}/linux/boot.asm ${LOOM_MOD_ARCH_SRC_DIR}/linux/linux.c)
create_module(ps2 ${LOOM_MOD_ARCH_SRC_DIR}/ps2.c)
create_module(serial ${LOOM_MOD_ARCH_SRC_DIR}/serial.c)