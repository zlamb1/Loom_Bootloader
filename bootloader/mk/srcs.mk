SRCS := commands/core.c \
		crypto/md5.c \
		arith.c \
		command.c \
	    console.c \
		disk.c \
		error.c \
		input.c \
		keycode.c \
		main.c \
		mm.c \
		panic.c \
		print.c \
		shell.c \
		sp.c \
		string.c \
		symbol.c 

SRCS := $(foreach src,$(SRCS),src/core/$(src))

TSRCS :=

ifeq ($(TARGET),i686)

TGTDIR := src/arch/$(TARGET)

TGTSRCS := stage1.asm \
	  	   stage2.asm \
		   disk.asm \
		   a20.asm \
		   tables.asm \
		   int.asm \
		   isr.asm \
		   arch.c \
		   vga.c \
		   pic.c \
		   io.c \
		   idt.c \
		   exception.c \
		   ps2.c \
		   bios.c

LNKSCRPT := $(TGTDIR)/loom.ld

endif

SRCS += $(foreach src,$(TGTSRCS),$(TGTDIR)/$(src))