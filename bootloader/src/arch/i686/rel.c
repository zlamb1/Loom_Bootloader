#include "loom/elf.h"
#include "loom/error.h"
#include "loom/string.h"

int
loom_elf32_rel_fixup (loom_elf32_rel_t *rel, loom_elf32_sym_t *sym, void *b)
{
  loom_int64_t I;
  loom_int32_t A, R;

  loom_uint8_t type = LOOM_ELF32_R_TYPE (rel->info);

  char *c = b;
  c += rel->offset;

  loom_memcpy (&A, c, sizeof (A));

  switch (type)
    {
#define R_386_NONE 0
#define R_386_32   1
#define R_386_PC32 2
    case R_386_NONE:
      return 0;
    case R_386_32:
      // S + A
      I = (loom_int64_t) sym->value + (loom_int64_t) A;
      break;
    case R_386_PC32:
      // S + A - P
      I = (loom_int64_t) sym->value + (loom_int64_t) A - (loom_address_t) c;
      break;
    default:
      loom_error (LOOM_ERR_BAD_ELF_REL, "Unsupported ELF relocation type %lu",
                  (unsigned long) type);
      return -1;
    }

  if (I < LOOM_INT32_MIN || I > LOOM_INT32_MAX)
    {
      loom_error (LOOM_ERR_BAD_ELF_REL, "Relocation overflows address space");
      return -1;
    }

  R = (loom_int32_t) I;
  loom_memcpy (c, &R, sizeof (R));

  return 0;
}