MODS ?= src/mods/hello.c
MODSYMS ?= $(OUTDIR)/mods.syms
MODULE_TABLE_BIN ?= $(OUTDIR)/module-table

MOBJS := $(foreach mod,$(MODS),$(OUTDIR)/$(basename $(mod)).mod)
MDEPS := $(MOBJS:.mod=.d)

$(MODULE_TABLE_BIN): src/build/module_table.c
	@mkdir -p $(dir $@)
	$(CC) -I$(INCDIR) $< -o $@

$(OUTDIR)/%.mod: %.c
	@mkdir -p $(dir $@)
	$(CROSS_CC) -DLOOM_MODULE $(CFLAGS) $< -o $@
	$(CROSS_OBJCOPY) --only-keep-debug $@ $(basename $@).syms
	$(CROSS_OBJCOPY) --strip-unneeded $@

ifeq ($(DEBUG),1)
$(MODSYMS): $(MOBJS)
	@mkdir -p $(dir $@)
	@rm -f $@
	@touch $@
	for mod in $(basename $<); do \
		echo $$(sha1sum $${mod}.mod | cut -d' ' -f1) $$(realpath $${mod}.syms) >>$@; \
	done
else
$(MODSYMS):
	@touch $@
endif

clean::
	rm -f $(MODSYMS) $(MODULE_TABLE_BIN) $(MDEPS) $(MOBJS)