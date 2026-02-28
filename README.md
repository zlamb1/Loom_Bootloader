# Loom Bootloader

Loom Bootloader is a modular bootloader, written in assembly and C, designed primarily to boot Linux.

## Supported Architectures
- x86 (BIOS)

## Build Dependencies
- cmake
- cc
- nasm
- i686-elf-gcc
- i686-elf-objcopy

## Build Instructions
- cmake -S . -B build
- cd build
- make *---OR---* ninja

## Testing Dependencies
- POSIX shell
- qemu-system-i386
- qemu-system-x86_64
- GDB
- Python

## Run Instructions
- To run the bootloader in QEMU, execute run in the **build directory**.
- To debug via GDB, execute dbg in the **build directory**.