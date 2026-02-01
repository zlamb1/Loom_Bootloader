# Loom Bootloader

Loom Bootloader is a modular bootloader, written in assembly and C, designed primarily to boot Linux.

## Supported Architectures
- x86 (BIOS)

## Build Dependencies
- make
- cc
- podman
- cut
- realpath

## Testing Dependencies
- qemu-system-i386
- qemu-system-x86_64
- GDB
- Python

## Commands
- make: Builds the bootloader.
- make dbg: Runs the bootloader in QEMU and attaches GDB.
- make qemu: Runs the bootloader in QEMU.
- make compile-commands: Builds the bootloader and produces compile-commands.json.
- make clean: Deletes intermediate files.