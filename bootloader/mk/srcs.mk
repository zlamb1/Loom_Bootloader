SRCS := arith.c \
	    console.c \
		main.c \
		mm.c \
		panic.c \
		print.c \
		shell.c \
		sp.c \
		symbol.c

SRCS := $(foreach src,$(SRCS),src/core/$(src))

TSRCS :=

ifeq ($(TARGET),i686)

TGTDIR := src/arch/$(TARGET)
TGTSRCS := stage1.asm stage2.asm disk.asm a20.asm tables.asm int.asm arch.c vga.c
LNKSCRPT := $(TGTDIR)/loom.ld

endif

SRCS += $(foreach src,$(TGTSRCS),$(TGTDIR)/$(src))