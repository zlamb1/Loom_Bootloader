INCDIR := include

HDRS := arch.h \
        console.h \
		error.h \
		main.h \
		mmap.h \
		mm.h \
		print.h \
		shell.h \
		sp.h \
		string.h \
		symbol.h \
		types.h

ifeq ($(TARGET),i686)

HDRS += arch/i686/bios.h \
		arch/i686/idt.h \
		arch/i686/io.h \
		arch/i686/isr.h \
		arch/i686/pic.h

endif

HDRS := $(foreach hdr,$(HDRS),$(INCDIR)/loom/$(hdr))

.PHONY: test
test:
	./mk/gensym.sh test.c $(HDRS)