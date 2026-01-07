OUTDIR ?= build
PODMAN ?= podman

MARKER := $(OUTDIR)/build.marker
TAG := loomos-bootloader

SRCS := src/arch/i686/stage1.asm src/arch/i686/stage2.asm src/core/main.c
OBJS := $(foreach src,$(SRCS),$(OUTDIR)/$(basename $(src)).o)
DEPS := $(OBJS:.o=.d)

LNK_SCRIPT := src/arch/i686/loom.ld

ELF  := $(OUTDIR)/bootloader.elf
BIN  := $(OUTDIR)/bootloader

NASM ?= nasm

ASMFLAGS ?= -gdwarf -f elf32

CROSS_CC ?= i686-elf-gcc

INCDIR := include

CFLAGS ?= -g -std=gnu11 -Os -ffreestanding -Wall -Wextra -pedantic \
		  -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wconversion -Wformat \
		  -Wuninitialized -Werror

# Required C flags.
CFLAGS += -c -fno-strict-aliasing -mno-red-zone -MMD -I$(INCDIR)

CROSS_LD ?= i686-elf-ld
CROSS_OBJCOPY ?= i686-elf-objcopy

.PHONY: all podman-build build podman-clean clean

all: podman-build

podman-build: $(MARKER)
	@mkdir -p $(OUTDIR)
	podman run -it --rm -v ./:/src $(TAG) make build

build: $(BIN)

$(BIN): $(ELF)
	$(CROSS_OBJCOPY) -S -g -O binary $< $@

$(ELF): $(DEPS) $(LNK_SCRIPT)
	$(CROSS_LD) $(LDFLAGS) -T $(LNK_SCRIPT) $(OBJS) -o $@

$(OUTDIR)/%.d: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASMFLAGS) -MD $@ $< -o $(basename $@).o

$(OUTDIR)/%.d: %.c
	@mkdir -p $(dir $@)
	$(CROSS_CC) $(CFLAGS) $< -o $(basename $@).o

$(MARKER):
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