MODDIR := src/mods
MODOUTDIR := $(OUTDIR)/mods

MODS ?= mods/hello.mk mods/x86/ps2.mk mods/x86/linux.mk
MODSYMLST := $(MODOUTDIR)/mods.sym.lst
MODULE_TABLE_BIN ?= $(OUTDIR)/module-table

MODOBJS :=
MODDEPS = $(MODOBJS:.o=.d)
MODSYMS :=
MODBINS :=

ABSPATH := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

define MAKE_MODULE

undefine MODNAME
undefine MODSRCS

include $(ABSPATH)$(1)

ifndef MODNAME
$$(error Module name not defined for $(1))
endif

ifndef MODSRCS
$$(error Module sources not defined for $$(MODNAME))
endif

OBJS := $$(foreach SRC,$$(MODSRCS),$(MODOUTDIR)/$$(basename $$(SRC)).c.o) \
		$$(foreach SRC,$$(MODASM),$(MODOUTDIR)/$$(basename $$(SRC)).asm.o)

MODOBJS += $$(OBJS)

MODSYMS += $(MODOUTDIR)/$$(MODNAME).syms

MODBIN = $(MODOUTDIR)/$$(MODNAME).mod
MODBINS += $$(MODBIN)

$$(filter %.c.o, $$(OBJS)): $(MODOUTDIR)/%.c.o: %.c
	@mkdir -p $$(dir $$@)
	$(CROSS_CC) -DLOOM_MODULE $(CFLAGS) $$< -o $$@

$$(filter %.asm.o, $$(OBJS)): $(MODOUTDIR)/%.asm.o: %.asm
	@mkdir -p $$(dir $$@)
	$(NASM) $(ASMFLAGS) -MD $$(basename $$@).d $$< -o $$@

$$(MODBIN): $$(OBJS)
	$(CROSS_CC) -r $$^ -o $$@
	$(CROSS_OBJCOPY) --only-keep-debug $$@ $$(basename $$@).syms
	$(CROSS_OBJCOPY) --strip-unneeded $$@

endef

$(foreach MOD,$(MODS),$(eval $(call MAKE_MODULE,$(MOD))))

$(MODULE_TABLE_BIN): src/build/module_table.c
	@mkdir -p $(dir $@)
	$(CC) -I$(INCDIR) $< -o $@

ifeq ($(DEBUG),1)
$(MODSYMLST): $(MODBINS)
	@mkdir -p $(dir $@)
	@rm -f $@
	@touch $@
	for mod in $(basename $^); do \
		echo $$(sha1sum $${mod}.mod | cut -d' ' -f1) $$(realpath $${mod}.syms) >>$@; \
	done
else
$(MODSYMLST):
	@touch $@
endif

clean::
	rm -f $(MODOBJS) $(MODDEPS) $(MODSYMS) $(MODBINS) $(MODSYMLST) $(MODULE_TABLE_BIN)

-include $(MODDEPS)