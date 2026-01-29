#include "loom/elf.h"
#include "loom/error.h"
#include "loom/math.h"

int
loom_elf32_ehdr_load (void *p, loom_usize_t size, loom_elf32_ehdr_t **ehdr)
{
  loom_elf32_ehdr_t *_ehdr;

  if (size < sizeof (*_ehdr))
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "ELF header size %lu too small",
                (ulong) size);

  _ehdr = p;

  if (_ehdr->size < sizeof (*_ehdr))
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "invalid ELF header size %lu",
                (ulong) _ehdr->size);

  if (_ehdr->magic != LOOM_EH_MAG0 || _ehdr->sig[0] != LOOM_EH_MAG1
      || _ehdr->sig[1] != LOOM_EH_MAG2 || _ehdr->sig[2] != LOOM_EH_MAG3)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "invalid ELF header magic");

  if (_ehdr->class != LOOM_EH_CLASS_32)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                "invalid ELF class 0x%lx (expected 32-bit)",
                (ulong) _ehdr->class);

#if defined(LOOM_LITTLE_ENDIAN)
  if (_ehdr->data != LOOM_EH_DATA_LE)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                "invalid ELF data encoding 0x%lx (expected little endian)",
                (ulong) _ehdr->data);
#elif defined(LOOM_BIG_ENDIAN)
  if (_ehdr->data != LOOM_EH_DATA_BE)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                "invalid ELF data encoding 0x%lx (expected big endian)",
                (ulong) _ehdr->data);
#else
#error Unsupported Endianness
#endif

  if (_ehdr->type == LOOM_ET_NONE)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "ELF type 'none' not supported");

  if (_ehdr->type == LOOM_ET_SHARED)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "ELF type 'shared' not supported");

  if (_ehdr->type == LOOM_ET_SHARED)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "ELF type 'core' not supported");

  if (_ehdr->type != LOOM_ET_REL && _ehdr->type != LOOM_ET_EXEC)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR, "unknown ELF type 0x%lx",
                (ulong) _ehdr->type);

  if (_ehdr->phents)
    {
      loom_address_t phoff;

      if (_ehdr->phentsize < sizeof (loom_elf32_phdr_t))
        LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                    "invalid program header entry size %lu",
                    (ulong) _ehdr->phentsize);

      if (loom_mul (_ehdr->phents, _ehdr->phentsize, &phoff)
          || loom_add (_ehdr->phoff, phoff, &phoff) || phoff > size)
        LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                    "invalid program header table size");
    }

  if (_ehdr->shents)
    {
      loom_address_t shoff;

      if (_ehdr->shentsize < sizeof (loom_elf32_shdr_t))
        LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                    "invalid section header entry size %lu",
                    (ulong) _ehdr->shentsize);

      if (loom_mul (_ehdr->shents, _ehdr->shentsize, &shoff)
          || loom_add (_ehdr->shoff, shoff, &shoff) || shoff > size)
        LOOM_ERROR (LOOM_ERR_BAD_ELF_EHDR,
                    "invalid section header table size");
    }

  *ehdr = _ehdr;

  return 0;
}

int
loom_elf32_shdr_validate (loom_address_t addr, loom_usize_t size,
                          loom_elf32_shdr_t *shdr)
{
  loom_address_t shoff;

  if (shdr->offset >= size)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR, "invalid section offset");

  if (loom_add (shdr->offset, shdr->size, &shoff)
      || loom_add (addr, shoff, &shoff))
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR, "section size overflows");

  if (shdr->type != LOOM_SHT_NOBITS && shdr->offset + shdr->size > size)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR, "section overflows ELF");

  return 0;
}

int
loom_elf32_strtab_validate (loom_elf32_ehdr_t *ehdr, loom_usize_t size,
                            loom_elf32_shdr_t *shdr)
{
  const char *strtab;

  if (loom_elf32_shdr_validate ((loom_address_t) ehdr, size, shdr))
    return -1;

  if (shdr->type != LOOM_SHT_STRTAB)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_STRTAB, "invalid type 0x%lx for string table",
                (ulong) shdr->type);

  strtab = (const char *) ((loom_address_t) ehdr + shdr->offset);

  if (!shdr->size)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_STRTAB, "invalid size 0 for string table");

  if (strtab[0])
    LOOM_ERROR (LOOM_ERR_BAD_ELF_STRTAB,
                "first element of string table not 0");

  if (strtab[shdr->size - 1])
    LOOM_ERROR (LOOM_ERR_BAD_ELF_STRTAB, "last element of string table not 0");

  return 0;
}

loom_elf32_shdr_t *
loom_elf32_shdr_get (loom_elf32_ehdr_t *ehdr, loom_usize_t shidx)
{
  loom_address_t addr;

  if (shidx >= ehdr->shents)
    return NULL;

  addr = (loom_address_t) ehdr + ehdr->shoff + shidx * ehdr->shentsize;
  return (loom_elf32_shdr_t *) addr;
}

int
loom_elf32_shdr_iterate (loom_elf32_ehdr_t *ehdr,
                         int (*hook) (loom_usize_t, loom_elf32_shdr_t *,
                                      void *),
                         void *data)
{
  loom_address_t addr;
  int retval;

  addr = (loom_address_t) ehdr + ehdr->shoff;

  for (unsigned int i = 0; i < ehdr->shents; ++i, addr += ehdr->shentsize)
    if ((retval = hook (i, (loom_elf32_shdr_t *) addr, data)))
      return retval;

  return 0;
}

typedef struct
{
  loom_elf32_ehdr_t *ehdr;
  int (*hook) (loom_elf32_rel_t *, loom_usize_t, loom_elf32_shdr_t *,
               loom_usize_t, loom_elf32_shdr_t *, void *);
  void *data;
} rel_iterate_hook_context_t;

static int
rel_iterate_hook (UNUSED loom_usize_t shidx, loom_elf32_shdr_t *shdr,
                  void *data)
{
  rel_iterate_hook_context_t *ctx = data;
  int retval;

  loom_address_t addr;
  loom_elf32_shdr_t *symtab, *target;

  if (shdr->type != LOOM_SHT_REL)
    return 0;

  if (shdr->entsize < sizeof (loom_elf32_rel_t))
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR, "invalid relocation entry size %lu",
                (ulong) shdr->entsize);

  if (!(symtab = loom_elf32_shdr_get (ctx->ehdr, shdr->link))
      || symtab->type != LOOM_SHT_SYMTAB)
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR,
                "relocation link is not a symbol table");

  if (!(target = loom_elf32_shdr_get (ctx->ehdr, shdr->info)))
    LOOM_ERROR (LOOM_ERR_BAD_ELF_SHDR, "invalid relocation target");

  addr = (loom_address_t) ctx->ehdr + shdr->offset;

  for (unsigned int i = 0; i < shdr->size / shdr->entsize;
       ++i, addr += shdr->entsize)
    if ((retval = ctx->hook ((loom_elf32_rel_t *) addr, shdr->link, symtab,
                             shdr->info, target, ctx->data)))
      return retval;

  return 0;
}

int
loom_elf32_rel_iterate (loom_elf32_ehdr_t *ehdr,
                        int (*hook) (loom_elf32_rel_t *, loom_usize_t,
                                     loom_elf32_shdr_t *, loom_usize_t,
                                     loom_elf32_shdr_t *, void *),
                        void *data)
{
  rel_iterate_hook_context_t ctx
      = { .ehdr = ehdr, .hook = hook, .data = data };
  return loom_elf32_shdr_iterate (ehdr, rel_iterate_hook, &ctx);
}