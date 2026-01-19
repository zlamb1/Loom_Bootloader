MODS ?= src/mods/hello.c
MODULE_TABLE_BIN ?= $(OUTDIR)/module-table

MOBJS := $(foreach mod,$(MODS),$(OUTDIR)/$(basename $(mod)).mod)
MDEPS := $(MOBJS:.mod=.d)

$(MODULE_TABLE_BIN): src/build/module_table.c
	@mkdir -p $(dir $@)
	$(CC) -I$(INCDIR) $< -o $@

$(OUTDIR)/%.mod: %.c
	@mkdir -p $(dir $@)
	$(CROSS_CC) -DLOOM_MODULE $(CFLAGS) $< -o $@
	$(CROSS_OBJCOPY) --strip-unneeded $@

clean::
	rm -f $(MODULE_TABLE_BIN) $(MDEPS) $(MOBJS)