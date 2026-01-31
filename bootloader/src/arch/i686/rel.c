#include "loom/elf.h"
#include "loom/error.h"
#include "loom/string.h"

#define R_386_NONE 0
#define R_386_32   1
#define R_386_PC32 2
#define R_386_16   20
#define R_386_PC16 21

static loom_int64_t
rel32_fixup (loom_uint8_t type, loom_int64_t S, loom_int64_t A, loom_int64_t P)
{
  switch (type)
    {
    case R_386_32:
      return S + A;
    case R_386_PC32:
      return S + A - P;
    }

  loom_panic ("rel32_fixup");
}

static loom_int32_t
rel16_fixup (loom_uint8_t type, loom_int32_t S, loom_int32_t A, loom_int32_t P)
{
  switch (type)
    {
    case R_386_16:
      return S + A;
    case R_386_PC16:
      return S + A - P;
    }

  loom_panic ("rel16_fixup");
}

int
loom_elf32_rel_fixup (loom_elf32_rel_t *rel, loom_elf32_sym_t *sym, void *b)
{
  loom_uint8_t type = LOOM_ELF32_R_TYPE (rel->info);

  char *c = b;
  c += rel->offset;

  switch (type)
    {
    case R_386_NONE:
      return 0;
    case R_386_32:
    case R_386_PC32:
      {
        loom_int64_t R64 = (loom_int64_t) sym->value;
        loom_int32_t R;
        loom_memcpy (&R, c, sizeof (R));
        R64 = rel32_fixup (type, R64, (loom_int64_t) R,
                           (loom_int64_t) (loom_address_t) c);
        if (R64 < LOOM_INT32_MIN || R64 > LOOM_INT32_MAX)
          {
            loom_error (LOOM_ERR_BAD_ELF_REL, "relocation truncated to fit");
            return -1;
          }
        R = (loom_int32_t) R64;
        loom_memcpy (c, &R, sizeof (R));
        break;
      }
    case R_386_16:
    case R_386_PC16:
      {
        loom_int32_t R32 = (loom_int32_t) sym->value;
        loom_int16_t R;
        loom_memcpy (&R, c, sizeof (R));
        R32 = rel16_fixup (type, R32, (loom_int32_t) R,
                           (loom_int32_t) (loom_address_t) c);
        if (R32 < LOOM_INT16_MIN || R32 > LOOM_INT16_MAX)
          {
            loom_error (LOOM_ERR_BAD_ELF_REL, "relocation truncated to fit");
            return -1;
          }
        R = (loom_int16_t) R32;
        loom_memcpy (c, &R, sizeof (R));
        break;
      }
    default:
      loom_error (LOOM_ERR_BAD_ELF_REL, "unsupported ELF relocation type %lu",
                  (unsigned long) type);
      return -1;
    }

  return 0;
}