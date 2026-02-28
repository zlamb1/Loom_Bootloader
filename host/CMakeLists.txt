cmake_minimum_required(VERSION 3.10)

project(loom-install LANGUAGES C)

set(
    CMAKE_C_FLAGS 
    "-g -O2 -std=gnu11 -ffreestanding -fstack-protector-all -Wall -Wextra \
     -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wconversion -Wformat \
	 -Wuninitialized -Werror -fno-strict-aliasing -fno-omit-frame-pointer \
	 -mno-red-zone"
)

set(SRCS install.c)
set(BIN "loom-install")

add_executable(
    ${BIN}
    ${SRCS}
)

target_include_directories(${BIN} PRIVATE ${LOOM_INCLUDE_DIRS})

install(TARGETS ${BIN} RUNTIME DESTINATION bin)