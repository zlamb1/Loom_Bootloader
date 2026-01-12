GENSYMS := mk/gensym.sh
SYMS := $(OUTDIR)/src/core/syms.gen

$(SYMS).c: $(GENSYMS) $(HDRS)
	sh $(GENSYMS) $@ $(HDRS)

$(SYMS).o: $(SYMS).c
	@mkdir -p $(dir $@)
	$(CROSS_CC) $(CFLAGS) $< -o $@

clean::
	rm -f $(SYMS).c

OBJS += $(SYMS).o