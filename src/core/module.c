#include "loom/module.h"
#include "loom/elf.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/print.h"
#include "loom/string.h"
#include "loom/symbol.h"

usize loom_modend;

struct strtab
{
  usize size;
  const char *strs;
};

struct symtab
{
  usize entsize;
  usize ents;
  usize shidx;
  loom_elf32_shdr *shdr;
  struct strtab strtab;
};

typedef struct
{
  loom_elf32_ehdr *ehdr;
  usize size;
  loom_module *mod;
  loom_module_section **sections;
  struct symtab *symtab;
  struct strtab *shstrtab;
} section_iterate_context;

typedef struct
{
  loom_elf32_ehdr *ehdr;
  loom_module_section **sections;
  struct symtab *symtab;
} rel_iterate_context;

loom_list loom_modules = LOOM_LIST_HEAD (loom_modules);

static int
sectionsIterate (usize shidx, loom_elf32_shdr *shdr, void *data)
{
  section_iterate_context *ctx = data;

  if (loomELF32ShdrValidate ((address) ctx->ehdr, ctx->size, shdr))
    return -1;

  if (shidx == LOOM_SHN_UNDEF && shdr->type != LOOM_SHT_NULL)
    {
      loomErrorFmt (LOOM_ERR_BAD_ELF_SHDR,
                    "undefined section has non-null type");
      return -1;
    }

  if (shdr->type == LOOM_SHT_SYMTAB)
    {
      loom_elf32_shdr *strtab;

      if (ctx->symtab->shidx != LOOM_SHN_UNDEF)
        {
          loomErrorFmt (LOOM_ERR_BAD_MODULE,
                        "multiple symbol tables not supported in module");
          return -1;
        }

      if (!(strtab = loomELF32ShdrGet (ctx->ehdr, shdr->link))
          || loomELF32StrTabValidate (ctx->ehdr, ctx->size, strtab))
        return -1;

      if (shdr->entsize < sizeof (loom_elf32_sym))
        {
          loomErrorFmt (LOOM_ERR_BAD_ELF_SHDR,
                        "invalid symbol table entry size %lu",
                        (unsigned long) shdr->entsize);
          return -1;
        }

      ctx->symtab->entsize = shdr->entsize;
      ctx->symtab->ents = shdr->size / shdr->entsize;
      ctx->symtab->shidx = shidx;
      ctx->symtab->shdr = shdr;
      ctx->symtab->strtab.size = strtab->size;
      ctx->symtab->strtab.strs
          = (const char *) ((address) ctx->ehdr + strtab->offset);
    }

  if (shdr->flags & LOOM_SHF_ALLOC)
    {
      loom_module_section *section;

      if (!shdr->size)
        return 0;

      if ((shdr->addralign & (shdr->addralign - 1)) != 0)
        LOOM_ERROR (LOOM_ERR_BAD_MODULE,
                    "section alignment %lu is not a power of 2",
                    (ulong) shdr->addralign);

      section = loomAlloc (sizeof (*section));
      if (!section)
        return -1;

      section->shidx = shidx;
      section->size = shdr->size;

#ifdef LOOM_DEBUG
      if (ctx->shstrtab->strs)
        {
          struct strtab *shstrtab = ctx->shstrtab;
          usize len;

          if (shdr->name >= shstrtab->size)
            {
              loomErrorFmt (LOOM_ERR_BAD_MODULE,
                            "invalid section name index in string table");
              return -1;
            }

          len = loomStrLength (shstrtab->strs + shdr->name);
          if (len >= LOOM_MODULE_SECTION_NAME_LEN)
            len = LOOM_MODULE_SECTION_NAME_LEN - 1;

          loomMemCopy (section->name, shstrtab->strs + shdr->name, len);
          section->name[len] = '\0';
        }
#endif

      section->p = loomMemAlign (shdr->size, shdr->addralign);
      section->next = ctx->mod->sections;

      if (!section->p)
        {
          loomFree (section);
          return -1;
        }

      if (shdr->type == LOOM_SHT_NOBITS)
        loomMemSet (section->p, 0, shdr->size);
      else
        loomMemCopy (section->p, (void *) ((address) ctx->ehdr + shdr->offset),
                     shdr->size);

      ctx->mod->sections = section;
      ctx->sections[shidx] = section;
    }

  return 0;
}

static int
symbolValidate (const char *name, loom_elf32_sym *sym, u8 type,
                loom_module_section **sections, usize shents)
{
  if (LOOM_ELF32_ST_TYPE (sym->info) != type)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "module symbol %s has incorrect type %lu", name,
                    (unsigned long) type);
      return -1;
    }

  if (sym->shidx == LOOM_SHN_UNDEF || sym->shidx >= shents)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "module symbol %s has an invalid section index", name);
      return -1;
    }

  if (!sections[sym->shidx])
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "symbol %s is in non-alloc section %lu", name,
                    (unsigned long) sym->shidx);
      return -1;
    }

  return 0;
}

static int
relIterate (loom_elf32_rel *rel, usize symtabidx,
            unused loom_elf32_shdr *symtab, usize targetidx,
            unused loom_elf32_shdr *target, void *data)
{
  usize symidx;
  address addr;
  loom_module_section *section;

  rel_iterate_context *ctx = data;

  symidx = LOOM_ELF32_R_SYM (rel->info);

  if (symidx >= ctx->symtab->ents)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "relocation references invalid symbol %lu",
                    (unsigned long) symidx);
      return -1;
    }

  if (symtabidx != ctx->symtab->shidx)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "relocation references different symbol table %lu",
                    (unsigned long) symtabidx);
      return -1;
    }

  section = ctx->sections[targetidx];

  if (!section)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE,
                    "relocation affects non-alloc section %lu",
                    (unsigned long) targetidx);
      return -1;
    }

  if (rel->offset > rel->offset + 4 || rel->offset + 4 > section->size)
    {
      loomErrorFmt (LOOM_ERR_BAD_ELF_REL, "relocation overflows section");
      return -1;
    }

  addr = (address) ctx->ehdr + ctx->symtab->shdr->offset;
  addr += symidx * ctx->symtab->entsize;

  return loomELF32RelFixup (rel, (loom_elf32_sym *) addr, section->p);
}

static int
loomModuleLoad (void *p, usize size)
{
  loom_elf32_ehdr *ehdr;
  loom_elf32_sym *name_sym = NULL, *init_sym = NULL, *deinit_sym = NULL;

  loom_module *mod = NULL;
  loom_module_section **sections = NULL;

  struct symtab symtab = { 0 };
  struct strtab shstrtab = { 0 };

  if (loomELF32EhdrLoad (p, size, &ehdr))
    return -1;

  if (ehdr->shstridx != LOOM_SHN_UNDEF)
    {
      loom_elf32_shdr *shdr = loomELF32ShdrGet (ehdr, ehdr->shstridx);

      if (!shdr)
        {
          loomErrorFmt (LOOM_ERR_BAD_MODULE,
                        "invalid section header string table index");
          return -1;
        }

      if (loomELF32StrTabValidate (ehdr, size, shdr))
        return -1;

      shstrtab.size = shdr->size;
      shstrtab.strs = (const char *) p + shdr->offset;
    }

  mod = loomAlloc (sizeof (*mod));
  sections = loomZeroAlloc (ehdr->shents * sizeof (*sections));

  if (!mod || !sections)
    goto error;

  mod->sections = NULL;

  {
    symtab.shidx = LOOM_SHN_UNDEF;

    section_iterate_context ctx = {
      .sections = sections,
      .ehdr = ehdr,
      .mod = mod,
      .size = size,
      .symtab = &symtab,
      .shstrtab = &shstrtab,
    };

    if (loomELF32ShdrIterate (ehdr, sectionsIterate, &ctx))
      goto error;

    if (symtab.shidx == LOOM_SHN_UNDEF)
      {
        loomErrorFmt (LOOM_ERR_BAD_MODULE, "module symbol table not found");
        goto error;
      }
  }

  {
    address addr = (address) ehdr;
    addr += symtab.shdr->offset;

    for (unsigned int i = 0; i < symtab.ents; ++i, addr += symtab.entsize)
      {
        loom_module_section *section;
        loom_elf32_sym *sym = (void *) addr;
        u32 tmp;
        const char *name;

        // Skip NULL symbol.
        if (!i)
          continue;

        if (sym->name >= symtab.strtab.size)
          {
            loomErrorFmt (LOOM_ERR_BAD_ELF_SHDR,
                          "symbol has invalid index into string table");
            goto error;
          }

        name = symtab.strtab.strs + sym->name;

        if (!loomStrCmp (name, "loom_mod_name"))
          {
            if (symbolValidate (name, sym, LOOM_STT_OBJECT, sections,
                                ehdr->shents))
              goto error;

            name_sym = sym;
          }
        else if (!loomStrCmp (name, "loomModInit")
                 || !loomStrCmp (name, "loomModDeinit"))
          {
            if (symbolValidate (name, sym, LOOM_STT_FUNC, sections,
                                ehdr->shents))
              goto error;

            if (!loomStrCmp (name, "loomModInit"))
              init_sym = sym;
            else
              deinit_sym = sym;
          }
        else if (sym->shidx == LOOM_SHN_UNDEF)
          {
            loom_symbol *symbol = loomSymbolLookup (name);

            if (!symbol)
              {
                loomErrorFmt (LOOM_ERR_BAD_MODULE, "symbol not found %s",
                              name);
                goto error;
              }

            sym->value = (u32) symbol->p;
            continue;
          }

        if (sym->shidx == LOOM_SHN_UNDEF || sym->shidx >= ehdr->shents)
          {
            loomErrorFmt (LOOM_ERR_BAD_MODULE,
                          "invalid section index %lu for symbol %s",
                          (ulong) sym->shidx, name);
            goto error;
          }

        section = sections[sym->shidx];
        if (!section)
          continue;

        if (loomAdd (sym->value, sym->size, &tmp))
          {
            loomErrorFmt (LOOM_ERR_BAD_MODULE,
                          "symbol %s offset overflows address space", name);
            goto error;
          }

        if (sym->value + sym->size > section->size)
          {
            loomErrorFmt (LOOM_ERR_BAD_MODULE, "symbol %s overflows section",
                          name);
            goto error;
          }

        if (loomAdd (sym->value, (u32) section->p, &tmp))
          {
            loomErrorFmt (LOOM_ERR_BAD_MODULE,
                          "symbol offset overflows address space");
            goto error;
          }

        sym->value = tmp;
      }
  }

  if (!name_sym)
    {
      loomErrorFmt (LOOM_ERR_BAD_MODULE, "module name not found");
      goto error;
    }

  {
    rel_iterate_context ctx = {
      .ehdr = ehdr,
      .sections = sections,
      .symtab = &symtab,
    };

    if (loomELF32RelIterate (ehdr, relIterate, &ctx))
      goto error;
  }

  loomMemCopy (&mod->name, (char *) name_sym->value, sizeof (&mod->name));

  if (init_sym)
    mod->init = (loom_module_init) init_sym->value;
  else
    mod->init = NULL;

  if (deinit_sym)
    mod->deinit = (loom_module_deinit) deinit_sym->value;
  else
    mod->deinit = NULL;

  loomFree (sections);

  loomModuleAdd (mod);

  return 0;

error:
  loomFree (sections);

  if (mod)
    loomModuleUnload (mod);

  return -1;
}

address
loomModEndGet (void)
{
  loom_module_header header;
  if (loom_modbase == null)
    return 0;
  loomMemCopy (&header, (void *) loom_modbase, sizeof (header));
  return loom_modbase + loomEndianLoad (header.size);
}

void
loomCoreModulesLoad (void)
{
  loom_module_header header;
  u32 *modtab, modsize;

  usize addr;

  if (loom_modbase == null)
    {
      loomLogLn ("warning: no core modules found");
      return;
    }

  loomMemCopy (&header, (void *) loom_modbase, sizeof (header));

  auto magic = loomEndianLoad (header.magic);
  if (magic != LOOM_MODULE_HEADER_MAGIC)
    loomPanic ("bad module header magic: 0x%lx", (unsigned long) magic);

  auto size = loomEndianLoad (header.size);
  if (size < LOOM_MODULE_HEADER_MIN_SIZE
      || loomAdd (loom_modbase, size, &loom_modend))
    loomPanic ("bad module header size: %lu", (unsigned long) size);

  auto taboff = loomEndianLoad (header.taboff);
  if (taboff >= size)
    loomPanic ("bad module header taboff: %lu", (unsigned long) taboff);

  auto modoff = loomEndianLoad (header.modoff);
  if (taboff >= modoff)
    loomPanic ("bad module header taboff: must be less than modoff");

  modtab = (u32 *) (loom_modbase + taboff);
  addr = loom_modbase + modoff;

  while (1)
    {
      if ((address) modtab + sizeof (*modtab) > loom_modbase + modoff)
        loomPanic ("bad module header table: overflows modules");

      loomMemCopy (&modsize, modtab, sizeof (u32));
      modsize = le32toh (modsize);

      if (!modsize)
        break;

      if (loomAdd (addr, modsize, &addr))
        loomPanic ("bad core module size %lu", (unsigned long) modsize);

      if (addr > loom_modend)
        loomPanic ("bad core module size %lu bytes past modules end",
                   (unsigned long) (addr - loom_modend));

      if (loomModuleLoad ((void *) (addr - modsize), modsize))
        {
          loomLogLn ("Failed to load module: %s", loomErrorGet ());
          loomErrorClear ();
        }

      ++modtab;
    }
}

void
loomModuleAdd (loom_module *module)
{
  loomListAdd (&loom_modules, &module->node);

  if (module->init)
    module->init ();
}

bool
loomModuleRemove (const char *name)
{
  loom_module *module;

  loom_list_for_each_entry (&loom_modules, module, node)
  {
    if (!loomStrCmp (name, module->name))
      {
        if (module->deinit)
          module->deinit ();

        loomListRemove (&module->node);
        loomModuleUnload (module);

        return 1;
      }
  }

  return 0;
}

void
loomModuleUnload (loom_module *mod)
{
  loom_module_section *section;

  section = mod->sections;

  while (section)
    {
      loom_module_section *next = section->next;
      loomFree (section->p);
      loomFree (section);
      section = next;
    }

  loomFree (mod);
}