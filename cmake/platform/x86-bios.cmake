include("${CMAKE_CURRENT_LIST_DIR}/x86-common.cmake")

set(LOOM_PLATFORM_HDR_DIR "${LOOM_ARCH_HDR_DIR}/bios")
set(LOOM_PLATFORM_SRC_DIR "${LOOM_ARCH_SRC_DIR}/bios")

set(LOOM_LINKER_SCRIPT "${LOOM_PLATFORM_SRC_DIR}/loom.ld")

list(
    APPEND
    LOOM_HDRS
    "${LOOM_PLATFORM_HDR_DIR}/bios.h"
)

list(
    APPEND 
    LOOM_SRCS
    "${LOOM_PLATFORM_SRC_DIR}/bios.c"
    "${LOOM_PLATFORM_SRC_DIR}/exception.c"
    "${LOOM_PLATFORM_SRC_DIR}/int.S"
    "${LOOM_PLATFORM_SRC_DIR}/platform.c"
    "${LOOM_PLATFORM_SRC_DIR}/reboot.S"
    "${LOOM_PLATFORM_SRC_DIR}/boot.S"
    "${LOOM_PLATFORM_SRC_DIR}/stage1_5.S"
    "${LOOM_PLATFORM_SRC_DIR}/isr.S"
    "${LOOM_PLATFORM_SRC_DIR}/vga.c"
)
