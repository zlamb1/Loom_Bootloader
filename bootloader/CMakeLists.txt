cmake_minimum_required(VERSION 3.10)

if (NOT CMAKE_BUILD_TYPE)
    # Default to debug build.
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

include(ExternalProject)

set(CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(X86_GCC_TC "${CMAKE_DIR}/tc/x86-gcc.cmake")

include(${CMAKE_DIR}/os.cmake)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${X86_GCC_TC}")
endif()

project(Loom_Bootloader LANGUAGES ASM_NASM C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LOOM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(LOOM_INCLUDE_DIRS "${LOOM_INCLUDE_DIR}")
set(LOOM_CORE_HDR_DIR "${LOOM_INCLUDE_DIR}/loom")
set(LOOM_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(LOOM_CORE_SRC_DIR "${LOOM_SRC_DIR}/core")
set(LOOM_C_FLAGS -g -Os -std=gnu11 -ffreestanding -fstack-protector-strong 
                 -fno-strict-aliasing -fno-omit-frame-pointer -mno-red-zone)
set(LOOM_LINK_FLAGS -static -nostdlib)
set(LOOM_EXPORT_SYMBOLS_SRC "${CMAKE_CURRENT_BINARY_DIR}/export_symbols.c")
set(LOOM_GENSYMS "${CMAKE_CURRENT_LIST_DIR}/cmake/gensyms.cmake")
set(LOOM_HOST_DIR ${CMAKE_BINARY_DIR}/host)
set(LOOM_INSTALL_BIN ${LOOM_HOST_DIR}/bin/loom-install)
set(LOOM_IMG "${CMAKE_BINARY_DIR}/loom.img")

set(RUN_SCRIPT "${CMAKE_BINARY_DIR}/run")
set(DBG_SCRIPT "${CMAKE_BINARY_DIR}/dbg")
set(GDB_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/gdb/config")
set(SCRIPT_PERMS OWNER_WRITE OWNER_READ OWNER_EXECUTE 
                 GROUP_READ GROUP_EXECUTE)
set(QEMU_COMMON "-drive format=raw,file=${LOOM_IMG} -nic none -m 512M")

list(APPEND LOOM_C_FLAGS
    -Wall -Wextra -Wundef -Wshadow 
    -Wpointer-arith -Wcast-align -Wconversion 
    -Wformat -Wuninitialized -Werror)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    list(APPEND LOOM_C_FLAGS -DLOOM_DEBUG)
endif()

set(
    LOOM_HDRS
    ${LOOM_CORE_HDR_DIR}/commands/core.h
    ${LOOM_CORE_HDR_DIR}/crypto/crypto.h
    ${LOOM_CORE_HDR_DIR}/crypto/sha1.h
    ${LOOM_CORE_HDR_DIR}/arch.h
    ${LOOM_CORE_HDR_DIR}/assert.h
    ${LOOM_CORE_HDR_DIR}/command.h
    ${LOOM_CORE_HDR_DIR}/compiler.h
    ${LOOM_CORE_HDR_DIR}/console.h
    ${LOOM_CORE_HDR_DIR}/disk.h
    ${LOOM_CORE_HDR_DIR}/elf.h
    ${LOOM_CORE_HDR_DIR}/endian.h
    ${LOOM_CORE_HDR_DIR}/error.h
    ${LOOM_CORE_HDR_DIR}/input.h
    ${LOOM_CORE_HDR_DIR}/kernel_loader.h
    ${LOOM_CORE_HDR_DIR}/keycode.h
    ${LOOM_CORE_HDR_DIR}/list.h
    ${LOOM_CORE_HDR_DIR}/main.h
    ${LOOM_CORE_HDR_DIR}/math.h
    ${LOOM_CORE_HDR_DIR}/mm.h
    ${LOOM_CORE_HDR_DIR}/mmap.h
    ${LOOM_CORE_HDR_DIR}/module.h
    ${LOOM_CORE_HDR_DIR}/partition.h
    ${LOOM_CORE_HDR_DIR}/print.h
    ${LOOM_CORE_HDR_DIR}/shell.h
    ${LOOM_CORE_HDR_DIR}/sp.h
    ${LOOM_CORE_HDR_DIR}/string.h
    ${LOOM_CORE_HDR_DIR}/symbol.h
    ${LOOM_CORE_HDR_DIR}/types.h
)

set(
    LOOM_SRCS
    ${LOOM_CORE_SRC_DIR}/commands/core.c
    ${LOOM_CORE_SRC_DIR}/crypto/sha1.c
    ${LOOM_CORE_SRC_DIR}/arith.c
    ${LOOM_CORE_SRC_DIR}/assert.c
    ${LOOM_CORE_SRC_DIR}/command.c
    ${LOOM_CORE_SRC_DIR}/console.c
    ${LOOM_CORE_SRC_DIR}/disk.c
    ${LOOM_CORE_SRC_DIR}/elf.c
    ${LOOM_CORE_SRC_DIR}/error.c
    ${LOOM_CORE_SRC_DIR}/input.c
    ${LOOM_CORE_SRC_DIR}/kernel_loader.c
    ${LOOM_CORE_SRC_DIR}/keycode.c
    ${LOOM_CORE_SRC_DIR}/list.c
    ${LOOM_CORE_SRC_DIR}/main.c
    ${LOOM_CORE_SRC_DIR}/mm.c
    ${LOOM_CORE_SRC_DIR}/mmap.c
    ${LOOM_CORE_SRC_DIR}/module.c
    ${LOOM_CORE_SRC_DIR}/panic.c
    ${LOOM_CORE_SRC_DIR}/partition.c
    ${LOOM_CORE_SRC_DIR}/print.c
    ${LOOM_CORE_SRC_DIR}/shell.c
    ${LOOM_CORE_SRC_DIR}/sp.c
    ${LOOM_CORE_SRC_DIR}/string.c
    ${LOOM_CORE_SRC_DIR}/symbol.c
)

include("${CMAKE_DIR}/create_module.cmake")

if ("${LOOM_ENDIANNESS}" STREQUAL "LITTLE")
    set(LOOM_ENDIAN_DEFINITION "LOOM_LITTLE_ENDIAN")
elseif("${LOOM_ENDIANNESS}" STREQUAL "BIG")
    set(LOOM_ENDIAN_DEFINITION "LOOM_BIG_ENDIAN")
elseif(NOT DEFINED LOOM_ENDIANNESS)
    message(FATAL_ERROR "Architecture ${LOOM_ARCH}'s tc file must defined LOOM_ENDIANNESS.")
else()
    message(FATAL_ERROR "Architecture ${LOOM_ARCH}'s tc file must set LOOM_ENDIANNESS to either LITTLE or BIG.")
endif()

if ("${LOOM_ARCH}" STREQUAL "x86")
    include("${CMAKE_DIR}/arch/x86.cmake")
else()
    message(FATAL_ERROR "Unsupported architecture.")
endif()

include("${CMAKE_DIR}/modules.cmake")

add_custom_command(
    OUTPUT ${LOOM_EXPORT_SYMBOLS_SRC}
    COMMAND ${CMAKE_COMMAND} -DOUTFILE=${LOOM_EXPORT_SYMBOLS_SRC} -DHEADERS="${LOOM_HDRS}" -P ${LOOM_GENSYMS}
    DEPENDS ${LOOM_HDRS} ${LOOM_GENSYMS}
)

add_custom_target(loom_gensyms ALL DEPENDS ${LOOM_EXPORT_SYMBOLS_SRC})

list(APPEND LOOM_SRCS ${LOOM_EXPORT_SYMBOLS_SRC})

include("${CMAKE_DIR}/core.cmake")

ExternalProject_Add(
    host
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/host
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${LOOM_HOST_DIR}"
    CMAKE_CACHE_ARGS -DLOOM_INCLUDE_DIRS:STRING=${LOOM_INCLUDE_DIRS}
    BUILD_BYPRODUCTS ${LOOM_INSTALL_BIN}
)

add_custom_command(
    OUTPUT ${LOOM_IMG}
    COMMAND ${LOOM_INSTALL_BIN} -i ${LOOM_CORE_BIN} -o ${LOOM_IMG} ${LOOM_MODULES}
    DEPENDS ${LOOM_CORE_BIN} ${LOOM_MODULES} ${LOOM_INSTALL_BIN}
    VERBATIM
)

add_custom_target("loom_img" ALL DEPENDS ${LOOM_IMG})

file(WRITE ${RUN_SCRIPT}
    "make -C ${CMAKE_BINARY_DIR}\n"
    "${RUN_QEMU} ${QEMU_COMMON}\n"
)

file(CHMOD ${RUN_SCRIPT} FILE_PERMISSIONS ${SCRIPT_PERMS})

file(GENERATE OUTPUT ${DBG_SCRIPT} CONTENT 
"make -C ${CMAKE_BINARY_DIR}\n\
${DBG_QEMU} ${QEMU_COMMON} -s -S &\n\
gdb -q ${LOOM_CORE_ELF_ABS} -ex \"source ${CMAKE_CURRENT_LIST_DIR}/gdb/mod.py\" -ex \"set \\$moddir = \\\"${LOOM_MOD_BIN_DIR}\\\"\" -x ${GDB_SCRIPT}\n"
FILE_PERMISSIONS ${SCRIPT_PERMS}
)