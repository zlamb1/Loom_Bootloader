#include "loom/elf.h"
#include "loom/error.h"

int
loom_elf32_ehdr_load (void *p, loom_usize_t size, loom_elf32_ehdr_t **ehdr)
{
  loom_elf32_ehdr_t *_ehdr;

  LOOM_ERROR (!p, LOOM_ERR_BAD_ARG, NULL);
  LOOM_ERROR (!ehdr, LOOM_ERR_BAD_ARG, NULL);

  LOOM_ERROR (size < sizeof (*_ehdr), LOOM_ERR_BAD_ELF_HDR,
              "ELF size too small for header");

  _ehdr = p;

  LOOM_ERROR (_ehdr->magic != LOOM_EH_MAGIC, LOOM_ERR_BAD_ELF_HDR,
              "ELF magic not found");

  LOOM_ERROR (_ehdr->sig[0] != 'E' || _ehdr->sig[1] != 'L'
                  || _ehdr->sig[2] != 'F',
              LOOM_ERR_BAD_ELF_HDR, "ELF signature not found");

  LOOM_ERROR (_ehdr->class != LOOM_EH_CLASS_32, LOOM_ERR_BAD_ELF_HDR,
              "bad ELF class: expected 32-bit ELF");

#if defined(LOOM_LITTLE_ENDIAN)
  LOOM_ERROR (_ehdr->data != LOOM_EH_DATA_LE, LOOM_ERR_BAD_ELF_HDR,
              "ELF data encoding not supported: expected little endian");
#elif defined(LOOM_BIG_ENDIAN)
  LOOM_ERROR (_ehdr->data != LOOM_EH_DATA_BE, LOOM_ERR_BAD_ELF_HDR,
              "ELF data encoding not supported: expected big endian");
#else
#error Unsupported Endianness
#endif

  LOOM_ERROR (_ehdr->type == LOOM_ET_NONE, LOOM_ERR_BAD_ELF_HDR,
              "ELF type not supported: none");

  LOOM_ERROR (_ehdr->type == LOOM_ET_SHARED, LOOM_ERR_BAD_ELF_HDR,
              "ELF type not supported: shared");

  LOOM_ERROR (_ehdr->type == LOOM_ET_CORE, LOOM_ERR_BAD_ELF_HDR,
              "ELF type not supported: core");

  LOOM_ERROR (_ehdr->type != LOOM_ET_REL && _ehdr->type != LOOM_ET_EXEC,
              LOOM_ERR_BAD_ELF_HDR, "ELF type unknown: %u", _ehdr->type);

  LOOM_ERROR (_ehdr->type == LOOM_ET_EXEC && !_ehdr->entry,
              LOOM_ERR_BAD_ELF_HDR, "bad ELF entry point for executable");

  if (_ehdr->phents)
    {
      LOOM_ERROR (_ehdr->phentsize < sizeof (loom_elf32_phdr_t),
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF program header entry size: %u", _ehdr->phentsize);

      LOOM_ERROR (_ehdr->phents > LOOM_ADDRESS_MAX / _ehdr->phentsize
                      || _ehdr->phoff > LOOM_ADDRESS_MAX
                                            - _ehdr->phents * _ehdr->phentsize,
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF program header table: %u entries x %u entry size",
                  _ehdr->phents, _ehdr->phentsize);

      LOOM_ERROR (_ehdr->phoff + _ehdr->phents * _ehdr->phentsize > size,
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF program header table: overflows file");
    }

  if (_ehdr->shents)
    {
      LOOM_ERROR (_ehdr->shentsize < sizeof (loom_elf32_shdr_t),
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF section header entry size: %u", _ehdr->shentsize);

      LOOM_ERROR (_ehdr->shents > LOOM_ADDRESS_MAX / _ehdr->shentsize
                      || _ehdr->shoff > LOOM_ADDRESS_MAX
                                            - _ehdr->shents * _ehdr->shentsize,
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF section header table: %u entries x %u entry size",
                  _ehdr->shents, _ehdr->shentsize);

      LOOM_ERROR (_ehdr->shoff + _ehdr->shents * _ehdr->shentsize > size,
                  LOOM_ERR_BAD_ELF_HDR,
                  "bad ELF section header table: overflows file");
    }

  LOOM_ERROR (_ehdr->size < sizeof (*_ehdr), LOOM_ERR_BAD_ELF_HDR,
              "bad ELF header size: %lu", (unsigned long) _ehdr->size);

  *ehdr = _ehdr;

  return 0;
}

int
loom_elf32_shdr_validate (loom_address_t addr, loom_usize_t size,
                          loom_elf32_shdr_t *shdr)
{
  LOOM_ERROR (!shdr, LOOM_ERR_BAD_ARG, NULL);

  LOOM_ERROR (shdr->offset >= size, LOOM_ERR_BAD_ELF_SHDR,
              "bad ELF section offset: %lu bytes past file",
              (unsigned long) (shdr->offset - size + 1));

  LOOM_ERROR (shdr->offset > LOOM_UINT32_MAX - shdr->size
                  || addr > LOOM_ADDRESS_MAX - (shdr->offset + shdr->size),
              LOOM_ERR_BAD_ELF_SHDR, "bad ELF section size: %lu is too large",
              (unsigned long) shdr->size);

  LOOM_ERROR (
      shdr->type != LOOM_SHT_NOBITS && shdr->offset + shdr->size > size,
      LOOM_ERR_BAD_ELF_SHDR, "bad ELF section size: %lu bytes past file",
      (unsigned long) (shdr->offset + shdr->size - size));

  LOOM_ERROR ((shdr->addralign & (shdr->addralign - 1)) != 0,
              LOOM_ERR_BAD_ELF_SHDR,
              "bad ELF section alignment: %lu is not a power of 2",
              (unsigned long) shdr->addralign);

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
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "invalid type for string table: %lu",
                  (unsigned long) shdr->type);
      return -1;
    }

  strtab = (const char *) ((loom_address_t) ehdr + shdr->offset);

  if (!shdr->size)
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "empty string table");
      return -1;
    }

  if (strtab[0])
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "first index in string table not 0");
      return -1;
    }

  if (strtab[shdr->size - 1])
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "last index in string table not 0");
      return -1;
    }

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
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "invalid relocation entry size: %lu",
                  (unsigned long) shdr->entsize);
      return -1;
    }

  if (!(symtab = loom_elf32_shdr_get (ctx->ehdr, shdr->link))
      || symtab->type != LOOM_SHT_SYMTAB)
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR,
                  "relocation link is not symbol table");
      return -1;
    }

  if (!(target = loom_elf32_shdr_get (ctx->ehdr, shdr->info)))
    {
      loom_error (LOOM_ERR_BAD_ELF_SHDR, "invalid relocation section");
      return -1;
    }

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