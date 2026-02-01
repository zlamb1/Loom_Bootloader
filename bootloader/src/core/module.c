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

loom_usize_t loom_modend;

typedef struct
{
  loom_usize_t size;
  const char *strs;
} strtab_t;

typedef struct
{
  loom_usize_t entsize;
  loom_usize_t ents;
  loom_usize_t shidx;
  loom_elf32_shdr_t *shdr;
  strtab_t strtab;
} symtab_t;

typedef struct
{
  loom_elf32_ehdr_t *ehdr;
  loom_usize_t size;
  loom_module_t *mod;
  loom_module_section_t **sections;
  symtab_t *symtab;
  strtab_t *shstrtab;
} section_iterate_context_t;

typedef struct
{
  loom_elf32_ehdr_t *ehdr;
  loom_module_section_t **sections;
  symtab_t *symtab;
} rel_iterate_context_t;

loom_module_t *loom_modules;

static int
section_iterate (loom_usize_t shidx, loom_elf32_shdr_t *shdr, void *data)
{
  section_iterate_context_t *ctx = data;

  if (loom_elf32_shdr_validate ((loom_address_t) ctx->ehdr, ctx->size, shdr))
    return -1;

  if (shidx == LOOM_SHN_UNDEF && shdr->type != LOOM_SHT_NULL)
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR,
                  "undefined section has non-null type");
      return -1;
    }

  if (shdr->type == LOOM_SHT_SYMTAB)
    {
      loom_elf32_shdr_t *strtab;

      if (ctx->symtab->shidx != LOOM_SHN_UNDEF)
        {
          loom_error (LOOM_ERR_BAD_MODULE,
                      "multiple symbol tables not supported in module");
          return -1;
        }

      if (!(strtab = loom_elf32_shdr_get (ctx->ehdr, shdr->link))
          || loom_elf32_strtab_validate (ctx->ehdr, ctx->size, strtab))
        return -1;

      if (shdr->entsize < sizeof (loom_elf32_sym_t))
        {
          loom_error (LOOM_ERR_BAD_ELF_SHDR,
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
          = (const char *) ((loom_address_t) ctx->ehdr + strtab->offset);
    }

  if (shdr->flags & LOOM_SHF_ALLOC)
    {
      loom_module_section_t *section;

      if (!shdr->size)
        return 0;

      if ((shdr->addralign & (shdr->addralign - 1)) != 0)
        LOOM_ERROR (LOOM_ERR_BAD_MODULE,
                    "section alignment %lu is not a power of 2",
                    (ulong) shdr->addralign);

      section = loom_malloc (sizeof (*section));
      if (!section)
        return -1;

      section->shidx = shidx;
      section->size = shdr->size;

#ifdef LOOM_DEBUG
      if (ctx->shstrtab->strs)
        {
          strtab_t *shstrtab = ctx->shstrtab;
          loom_usize_t len;

          if (shdr->name >= shstrtab->size)
            {
              loom_error (LOOM_ERR_BAD_MODULE,
                          "invalid section name index in string table");
              return -1;
            }

          len = loom_strlen (shstrtab->strs + shdr->name);
          if (len >= LOOM_MODULE_SECTION_NAME_LEN)
            len = LOOM_MODULE_SECTION_NAME_LEN - 1;

          loom_memcpy (section->name, shstrtab->strs + shdr->name, len);
          section->name[len] = '\0';
        }
#endif

      section->p = loom_memalign (shdr->size, shdr->addralign);
      section->next = ctx->mod->sections;

      if (!section->p)
        {
          loom_free (section);
          return -1;
        }

      if (shdr->type == LOOM_SHT_NOBITS)
        loom_memset (section->p, 0, shdr->size);
      else
        loom_memcpy (section->p,
                     (void *) ((loom_address_t) ctx->ehdr + shdr->offset),
                     shdr->size);

      ctx->mod->sections = section;
      ctx->sections[shidx] = section;
    }

  return 0;
}

static int
symbol_validate (const char *name, loom_elf32_sym_t *sym, loom_uint8_t type,
                 loom_module_section_t **sections, loom_usize_t shents)
{
  if (LOOM_ELF32_ST_TYPE (sym->info) != type)
    {
      loom_error (LOOM_ERR_BAD_MODULE,
                  "module symbol %s has incorrect type %lu", name,
                  (unsigned long) type);
      return -1;
    }

  if (sym->shidx == LOOM_SHN_UNDEF || sym->shidx >= shents)
    {
      loom_error (LOOM_ERR_BAD_MODULE,
                  "module symbol %s has an invalid section index", name);
      return -1;
    }

  if (!sections[sym->shidx])
    {
      loom_error (LOOM_ERR_BAD_MODULE, "symbol %s is in non-alloc section %lu",
                  name, (unsigned long) sym->shidx);
      return -1;
    }

  return 0;
}

static int
rel_iterate (loom_elf32_rel_t *rel, loom_usize_t symtabidx,
             LOOM_UNUSED loom_elf32_shdr_t *symtab, loom_usize_t targetidx,
             LOOM_UNUSED loom_elf32_shdr_t *target, void *data)
{
  loom_usize_t symidx;
  loom_address_t addr;
  loom_module_section_t *section;

  rel_iterate_context_t *ctx = data;

  symidx = LOOM_ELF32_R_SYM (rel->info);

  if (symidx >= ctx->symtab->ents)
    {
      loom_error (LOOM_ERR_BAD_MODULE,
                  "relocation references invalid symbol %lu",
                  (unsigned long) symidx);
      return -1;
    }

  if (symtabidx != ctx->symtab->shidx)
    {
      loom_error (LOOM_ERR_BAD_MODULE,
                  "relocation references different symbol table %lu",
                  (unsigned long) symtabidx);
      return -1;
    }

  section = ctx->sections[targetidx];

  if (!section)
    {
      loom_error (LOOM_ERR_BAD_MODULE,
                  "relocation affects non-alloc section %lu",
                  (unsigned long) targetidx);
      return -1;
    }

  if (rel->offset > rel->offset + 4 || rel->offset + 4 > section->size)
    {
      loom_error (LOOM_ERR_BAD_ELF_REL, "relocation overflows section");
      return -1;
    }

  addr = (loom_address_t) ctx->ehdr + ctx->symtab->shdr->offset;
  addr += symidx * ctx->symtab->entsize;

  return loom_elf32_rel_fixup (rel, (loom_elf32_sym_t *) addr, section->p);
}

static int
loom_module_load (void *p, loom_usize_t size)
{
  loom_elf32_ehdr_t *ehdr;
  loom_elf32_sym_t *name_sym = NULL, *init_sym = NULL, *deinit_sym = NULL;

  loom_module_t *mod = NULL;
  loom_module_section_t **sections = NULL;

  symtab_t symtab = { 0 };
  strtab_t shstrtab = { 0 };

  if (loom_elf32_ehdr_load (p, size, &ehdr))
    return -1;

  if (ehdr->shstridx != LOOM_SHN_UNDEF)
    {
      loom_elf32_shdr_t *shdr = loom_elf32_shdr_get (ehdr, ehdr->shstridx);

      if (!shdr)
        {
          loom_error (LOOM_ERR_BAD_MODULE,
                      "invalid section header string table index");
          return -1;
        }

      if (loom_elf32_strtab_validate (ehdr, size, shdr))
        return -1;

      shstrtab.size = shdr->size;
      shstrtab.strs = (const char *) p + shdr->offset;
    }

  mod = loom_malloc (sizeof (*mod));
  sections = loom_zalloc (ehdr->shents * sizeof (*sections));

  if (!mod || !sections)
    goto error;

  mod->sections = NULL;

#ifdef LOOM_DEBUG
  loom_sha1_hash (size, p, mod->hash);
#endif

  {
    symtab.shidx = LOOM_SHN_UNDEF;

    section_iterate_context_t ctx = {
      .sections = sections,
      .ehdr = ehdr,
      .mod = mod,
      .size = size,
      .symtab = &symtab,
      .shstrtab = &shstrtab,
    };

    if (loom_elf32_shdr_iterate (ehdr, section_iterate, &ctx))
      goto error;

    if (symtab.shidx == LOOM_SHN_UNDEF)
      {
        loom_error (LOOM_ERR_BAD_MODULE, "module symbol table not found");
        goto error;
      }
  }

  {
    loom_address_t addr = (loom_address_t) ehdr;
    addr += symtab.shdr->offset;

    for (unsigned int i = 0; i < symtab.ents; ++i, addr += symtab.entsize)
      {
        loom_module_section_t *section;
        loom_elf32_sym_t *sym = (void *) addr;
        loom_uint32_t tmp;
        const char *name;

        // Skip NULL symbol.
        if (!i)
          continue;

        if (sym->name >= symtab.strtab.size)
          {
            loom_error (LOOM_ERR_BAD_ELF_SHDR,
                        "symbol has invalid index into string table");
            goto error;
          }

        name = symtab.strtab.strs + sym->name;

        if (!loom_strcmp (name, "loom_mod_name"))
          {
            if (symbol_validate (name, sym, LOOM_STT_OBJECT, sections,
                                 ehdr->shents))
              goto error;

            name_sym = sym;
          }
        else if (!loom_strcmp (name, "loom_mod_init")
                 || !loom_strcmp (name, "loom_mod_deinit"))
          {
            if (symbol_validate (name, sym, LOOM_STT_FUNC, sections,
                                 ehdr->shents))
              goto error;

            if (!loom_strcmp (name, "loom_mod_init"))
              init_sym = sym;
            else
              deinit_sym = sym;
          }
        else if (sym->shidx == LOOM_SHN_UNDEF)
          {
            loom_symbol_t *symbol = loom_symbol_lookup (name);

            if (!symbol)
              {
                loom_error (LOOM_ERR_BAD_MODULE, "symbol not found %s", name);
                goto error;
              }

            sym->value = (loom_uint32_t) symbol->p;
            continue;
          }

        if (sym->shidx == LOOM_SHN_UNDEF || sym->shidx >= ehdr->shents)
          {
            loom_error (LOOM_ERR_BAD_MODULE,
                        "invalid section index %lu for symbol %s",
                        (unsigned long) sym->shidx, name);
            goto error;
          }

        section = sections[sym->shidx];
        if (!section)
          continue;

        if (loom_add (sym->value, sym->size, &tmp))
          {
            loom_error (LOOM_ERR_BAD_MODULE,
                        "symbol %s offset overflows address space", name);
            goto error;
          }

        if (sym->value + sym->size > section->size)
          {
            loom_error (LOOM_ERR_BAD_MODULE, "symbol %s overflows section",
                        name);
            goto error;
          }

        if (loom_add (sym->value, (loom_uint32_t) section->p, &tmp))
          {
            loom_error (LOOM_ERR_BAD_MODULE,
                        "symbol offset overflows address space");
            goto error;
          }

        sym->value = tmp;
      }
  }

  if (!name_sym)
    {
      loom_error (LOOM_ERR_BAD_MODULE, "module name not found");
      goto error;
    }

  {
    rel_iterate_context_t ctx = {
      .ehdr = ehdr,
      .sections = sections,
      .symtab = &symtab,
    };

    if (loom_elf32_rel_iterate (ehdr, rel_iterate, &ctx))
      goto error;
  }

  loom_memcpy (&mod->name, (char *) name_sym->value, sizeof (&mod->name));

  if (init_sym)
    mod->init = (loom_module_init_t) init_sym->value;
  else
    mod->init = NULL;

  if (deinit_sym)
    mod->deinit = (loom_module_deinit_t) deinit_sym->value;
  else
    mod->deinit = NULL;

  mod->prev = NULL;
  mod->next = NULL;

  loom_free (sections);

  loom_module_add (mod);

  return 0;

error:
  loom_free (sections);
  if (mod)
    loom_module_unload (mod);
  return -1;
}

loom_address_t
loom_modend_get (void)
{
  loom_module_header_t hdr;
  loom_memcpy (&hdr, (void *) loom_modbase, sizeof (hdr));
  return loom_modbase + loom_le32toh (hdr.size);
}

void
loom_core_modules_load (void)
{
  loom_module_header_t hdr;
  loom_uint32_t *modtab, modsize;

  loom_usize_t addr;

  if (!loom_modbase)
    loom_panic ("loom_modbase not initialized");

  loom_memcpy (&hdr, (void *) loom_modbase, sizeof (hdr));

  hdr.magic = loom_le32toh (hdr.magic);
  hdr.taboff = loom_le32toh (hdr.taboff);
  hdr.modoff = loom_le32toh (hdr.modoff);
  hdr.size = loom_le32toh (hdr.size);

  if (hdr.magic != LOOM_MODULE_HEADER_MAGIC)
    loom_panic ("bad module header magic: 0x%lx", (unsigned long) hdr.magic);

  if (hdr.size < LOOM_MODULE_HEADER_MIN_SIZE
      || loom_add (loom_modbase, hdr.size, &loom_modend))
    loom_panic ("bad module header size: %lu", (unsigned long) hdr.size);

  if (hdr.taboff >= hdr.size)
    loom_panic ("bad module header taboff: %lu", (unsigned long) hdr.taboff);

  if (hdr.taboff >= hdr.modoff)
    loom_panic ("bad module header taboff: must be less than modoff");

  modtab = (loom_uint32_t *) (loom_modbase + hdr.taboff);
  addr = loom_modbase + hdr.modoff;

  while (1)
    {
      if ((loom_address_t) modtab + sizeof (*modtab)
          > loom_modbase + hdr.modoff)
        loom_panic ("bad module header table: overflows modules");

      loom_memcpy (&modsize, modtab, sizeof (loom_uint32_t));
      modsize = loom_le32toh (modsize);

      if (!modsize)
        break;

      if (loom_add (addr, modsize, &addr))
        loom_panic ("bad core module size %lu", (unsigned long) modsize);

      if (addr > loom_modend)
        loom_panic ("bad core module size %lu bytes past modules end",
                    (unsigned long) (addr - loom_modend));

      if (loom_module_load ((void *) (addr - modsize), modsize))
        loom_printf ("loom_module_load: %s\n", loom_error_get ());

      ++modtab;
    }
}

void
loom_module_add (loom_module_t *module)
{
  module->prev = NULL;
  module->next = loom_modules;

  if (loom_modules)
    loom_modules->prev = module;

  loom_modules = module;

  if (module->init)
    module->init ();
}

loom_bool_t
loom_module_remove (const char *name)
{
  LOOM_LIST_ITERATE (loom_modules, module)
  {
    if (!loom_strcmp (name, module->name))
      {
        if (module->deinit)
          module->deinit ();

        if (module->prev)
          module->prev->next = module->next;

        if (module->next)
          module->next->prev = module->prev;

        if (module == loom_modules)
          loom_modules = module->next;

        loom_module_unload (module);
        return 1;
      }
  }

  return 0;
}

void
loom_module_unload (loom_module_t *mod)
{
  loom_module_section_t *section;

  section = mod->sections;

  while (section)
    {
      loom_module_section_t *next = section->next;
      loom_free (section->p);
      loom_free (section);
      section = next;
    }

  loom_free (mod);
}