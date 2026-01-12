#include "loom/symbol.h"
#include "loom/types.h"

static loom_uint16_t size = 0;
static loom_symbol_t symtab[CSYMTAB];

int
EXPORT (loom_register_symbol) (const char *name, void *p)
{
  if (size >= sizeof (symtab) / sizeof (*symtab))
    return 0;

  symtab[size++] = (loom_symbol_t) { .name = name, .p = p };

  return 1;
}

loom_symbol_t *
loom_find_symbol (void *p)
{
  loom_symbol_t *nearest = NULL;

  for (int i = 0; i < size; i++)
    {
      loom_symbol_t *symbol = symtab + i;

      if (p < symbol->p)
        continue;

      if (!nearest || symbol->p > nearest->p)
        nearest = symbol;
    }

  return nearest;
}