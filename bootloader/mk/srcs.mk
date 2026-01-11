SRCS := main.c \
	    console.c \
		print.c \
		arith.c \
		shell.c \
		panic.c

SRCS := $(foreach src,$(SRCS),src/core/$(src))

TSRCS :=

ifeq ($(TARGET),i686)

TGTDIR := src/arch/$(TARGET)
TGTSRCS := stage1.asm stage2.asm disk.asm a20.asm tables.asm int.asm arch.c vga.c
LNKSCRPT := $(TGTDIR)/loom.ld

endif

SRCS += $(foreach src,$(TGTSRCS),$(TGTDIR)/$(src))