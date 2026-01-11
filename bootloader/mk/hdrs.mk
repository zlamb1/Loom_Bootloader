INCDIR := include/

HDRS := arch.h \
        console.h \
		error.h \
		main.h \
		mmap.h \
		print.h \
		shell.h \
		string.h \
		types.h

ifeq ($(TARGET),i686)

HDRS += arch/i686/bios.h

endif

HDRS := $(foreach $(hdr),$(HDRS),$(INCDIR)/$(hdr))