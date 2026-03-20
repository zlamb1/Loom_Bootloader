cmake_minimum_required(VERSION 3.10)

project(loom_utils LANGUAGES C)

set(
    CMAKE_C_FLAGS 
    "-g -O2 -std=gnu11 -ffreestanding -fstack-protector-all -Wall -Wextra \
     -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wconversion -Wformat \
	 -Wuninitialized -Werror -fno-strict-aliasing -fno-omit-frame-pointer \
	 -mno-red-zone"
)

set(INSTALL_SRCS install.c osdep.c host_impl.c 
                 ${LOOM_SRC_DIR}/mods/mbr.c ${LOOM_SRC_DIR}/core/partition.c 
                 ${LOOM_SRC_DIR}/core/list.c ${LOOM_SRC_DIR}/core/block_dev.c)
set(INSTALL_BIN "loom-install")
set(MKIMAGE_SRCS mkimage.c osdep.c)
set(MKIMAGE_BIN "loom-mkimage")

add_executable(
    ${MKIMAGE_BIN}
    ${MKIMAGE_SRCS}
)

add_executable(
    ${INSTALL_BIN}
    ${INSTALL_SRCS}
)

target_compile_definitions(${INSTALL_BIN} PRIVATE LOOM_UTIL)
target_compile_definitions(${MKIMAGE_BIN} PRIVATE LOOM_UTIL)

target_include_directories(${INSTALL_BIN} PRIVATE ${LOOM_INCLUDE_DIRS})
target_include_directories(${MKIMAGE_BIN} PRIVATE ${LOOM_INCLUDE_DIRS})

install(TARGETS ${INSTALL_BIN} RUNTIME DESTINATION bin)
install(TARGETS ${MKIMAGE_BIN} RUNTIME DESTINATION bin)