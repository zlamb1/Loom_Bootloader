TARGET := i686
NASM ?= nasm
CROSS_CC ?= $(TARGET)-elf-gcc
CROSS_OBJCOPY ?= $(TARGET)-elf-objcopy
CROSS_NM ?= $(TARGET)-elf-nm

ASMFLAGS ?= -gdwarf

# Required NASM flags.
ASMFLAGS += -f elf32

CFLAGS ?= -g -Os -std=gnu11 -ffreestanding -fstack-protector-strong -Wall -Wextra \
		  -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wconversion -Wformat \
		  -Wuninitialized -Werror

# Required C flags.
INC = -I$(INCDIR)
CFLAGS += -c -fno-strict-aliasing -fno-omit-frame-pointer \
	      -mno-red-zone -MMD $(INC)

ifeq ($(TARGET),i686)
CFLAGS += -DLOOM_LITTLE_ENDIAN
endif

LNKFLAGS ?=

# Required link flags.
LNKFLAGS += -nostdlib