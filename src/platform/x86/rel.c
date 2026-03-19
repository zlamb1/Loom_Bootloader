#include "loom/elf.h"
#include "loom/error.h"
#include "loom/string.h"
#include "loom/types.h"

#define R_386_NONE 0
#define R_386_32   1
#define R_386_PC32 2
#define R_386_16   20
#define R_386_PC16 21

static i32
rel16Fixup (u8 type, i32 S, i32 A, i32 P)
{
  switch (type)
    {
    case R_386_16:
      return S + A;
    case R_386_PC16:
      return S + A - P;
    }

  loomPanic ("rel16_fixup");
}

static i64
rel32Fixup (u8 type, i64 S, i64 A, i64 P)
{
  switch (type)
    {
    case R_386_32:
      return S + A;
    case R_386_PC32:
      return S + A - P;
    }

  loomPanic ("rel32_fixup");
}

int
loomELF32RelFixup (loom_elf32_rel *rel, loom_elf32_sym *sym, void *b)
{
  u8 type = LOOM_ELF32_R_TYPE (rel->info);

  char *c = b;
  c += rel->offset;

  switch (type)
    {
    case R_386_NONE:
      return 0;
    case R_386_16:
    case R_386_PC16:
      {
        i32 R32 = (i32) sym->value;
        i16 R;
        loomMemCopy (&R, c, sizeof (R));
        R32 = rel16Fixup (type, R32, (i32) R, (i32) (address) c);
        if (R32 < I16_MIN || R32 > I16_MAX)
          {
            loomErrorFmt (LOOM_ERR_BAD_ELF_REL, "relocation truncated to fit");
            return -1;
          }
        R = (i16) R32;
        loomMemCopy (c, &R, sizeof (R));
        break;
      }
    case R_386_32:
    case R_386_PC32:
      {
        i64 R64 = (i64) sym->value;
        i32 R;
        loomMemCopy (&R, c, sizeof (R));
        R64 = rel32Fixup (type, R64, (i64) R, (i64) (address) c);
        if (R64 < I32_MIN || R64 > I32_MAX)
          {
            loomErrorFmt (LOOM_ERR_BAD_ELF_REL, "relocation truncated to fit");
            return -1;
          }
        R = (i32) R64;
        loomMemCopy (c, &R, sizeof (R));
        break;
      }
    default:
      loomErrorFmt (LOOM_ERR_BAD_ELF_REL,
                    "unsupported ELF relocation type %lu",
                    (unsigned long) type);
      return -1;
    }

  return 0;
}