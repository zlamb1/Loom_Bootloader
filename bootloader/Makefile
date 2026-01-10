OUTDIR ?= build
PODMAN ?= podman

MARKER := $(OUTDIR)/build.marker
TAG := loomos-bootloader

TARGET := i686
TARGET_DIR := src/arch/$(TARGET)
TARGET_SRCS := stage1.asm stage2.asm disk.asm a20.asm tables.asm arch.c vga.c
TARGET_SRCS := $(foreach src,$(TARGET_SRCS),$(TARGET_DIR)/$(src))

CORE_SRCS := main.c console.c print.c arith.c
CORE_SRCS := $(foreach src,$(CORE_SRCS),src/core/$(src))

SRCS := $(TARGET_SRCS) $(CORE_SRCS)
OBJS := $(foreach src,$(SRCS),$(OUTDIR)/$(basename $(src)).o)
DEPS := $(OBJS:.o=.d)

LNK_SCRIPT := $(TARGET_DIR)/loom.ld

ELF  := $(OUTDIR)/bootloader.elf
BIN  := $(OUTDIR)/bootloader

NASM ?= nasm

ASMFLAGS ?= -gdwarf

# Required NASM flags.
ASMFLAGS += -f elf32

CROSS_CC ?= $(TARGET)-elf-gcc

INCDIR := include

CFLAGS ?= -g -std=gnu11 -Os -ffreestanding -Wall -Wextra \
		  -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wconversion -Wformat \
		  -Wuninitialized -Werror

# Required C flags.
CFLAGS += -c -fno-strict-aliasing -mno-red-zone -MMD -I$(INCDIR)

CROSS_LD ?= $(TARGET)-elf-ld
CROSS_OBJCOPY ?= $(TARGET)-elf-objcopy

.PHONY: all build clean compile-commands podman-build podman-clean podman-compile-commands

all: podman-build

podman-build: $(MARKER)
	podman run -it --rm -v ./:/src $(TAG) make build

build: $(BIN)

compile-commands:
	bear --output $(OUTDIR)/compile_commands.json -- make -B build

podman-compile-commands:
	podman run -it --rm -v ./:/src $(TAG) make compile-commands

$(BIN): $(ELF)
	$(CROSS_OBJCOPY) -S -g -O binary $< $@

$(ELF): $(OBJS) $(LNK_SCRIPT)
	$(CROSS_LD) $(LDFLAGS) -T $(LNK_SCRIPT) $(OBJS) -o $@

.DELETE_ON_ERROR:

$(OUTDIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASMFLAGS) -MD $(basename $@).d $< -o $@

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CROSS_CC) $(CFLAGS) $< -o $@

$(MARKER):
	@mkdir -p $(OUTDIR)
	podman build -t $(TAG) -f ./build.containerfile .
	touch $@

qemu: podman-build
	qemu-system-i386 -drive format=raw,file=$(BIN) -nic none -m 64M

debug: podman-build
	qemu-system-i386 -drive format=raw,file=$(BIN) -nic none -m 64M -s -S &
	gdb -q $(ELF) -x gdb/config

podman-clean:
	podman image rm $(TAG)

clean:
	rm -rf $(OUTDIR)

-include $(DEPS)