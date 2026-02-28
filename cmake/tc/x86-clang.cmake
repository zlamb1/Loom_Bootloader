include(${CMAKE_CURRENT_LIST_DIR}/x86-common.cmake)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET i686-unknown-none-elf)
set(CMAKE_OBJCOPY llvm-objcopy)