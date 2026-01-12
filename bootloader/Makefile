OUTDIR ?= build
BUILD_TYPE ?= podman

include mk/container.mk
include mk/toolchain.mk
include mk/hdrs.mk
include mk/srcs.mk

OBJS := $(foreach src,$(SRCS),$(OUTDIR)/$(basename $(src)).o)
DEPS := $(OBJS:.o=.d)

include mk/syms.mk

ELF   := $(OUTDIR)/bootloader.elf
BIN   := $(OUTDIR)/bootloader

.PHONY: all build clean compile-commands dbg

.DEFAULT_GOAL := all

all: build

ifeq ($(BUILD_TYPE),podman)

build: $(MARKER)
	$(RUN) make build

compile-commands:
	$(RUN) make compile-commands

else

build: $(BIN)

compile-commands:
	bear --output $(OUTDIR)/compile_commands.json -- make -B build

endif

$(BIN): $(ELF)
	$(CROSS_OBJCOPY) -O binary $< $@

$(ELF): $(OBJS) $(LNKSCRPT)
	$(CROSS_CC) $(LNKFLAGS) -T $(LNKSCRPT) $(OBJS) -o $@

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
	@touch $@

qemu: build
	qemu-system-i386 -drive format=raw,file=$(BIN) -nic none -m 64M

dbg: build
	qemu-system-i386 -drive format=raw,file=$(BIN) -nic none -m 64M -s -S &
	gdb -q $(ELF) -x gdb/config

clean::
	rm -f $(ELF) $(BIN) $(OBJS)

-include $(DEPS)