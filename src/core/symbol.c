#include "loom/symbol.h"
#include "loom/mm.h"
#include "loom/string.h"
#include "loom/types.h"

loom_symtab_t loom_symtab;

int
LOOM_EXPORT (loom_symbol_register) (const char *name, loom_bool_t isfunc,
                                    void *p)
{
  loom_symbol_t *symbol;

  if (loom_symtab.length >= LOOM_SYMTAB_SIZE)
    return 0;

  symbol = loom_malloc (sizeof (loom_symbol_t));
  if (!symbol)
    return 0;

  symbol->name = name;
  symbol->isfunc = isfunc;
  symbol->p = p;

  loom_symtab.symbols[loom_symtab.length++] = symbol;

  return 1;
}

loom_symbol_t *
loom_symbol_find (void *p)
{
  loom_symbol_t *nearest = NULL;

  for (loom_usize_t i = 0; i < loom_symtab.length; i++)
    {
      loom_symbol_t *symbol = loom_symtab.symbols[i];

      if (p < symbol->p)
        continue;

      if (!nearest || symbol->p > nearest->p)
        nearest = symbol;
    }

  return nearest;
}

loom_symbol_t *
loom_symbol_lookup (const char *name)
{
  for (unsigned int i = 0; i < loom_symtab.length; ++i)
    {
      loom_symbol_t *symbol = loom_symtab.symbols[i];

      if (!loom_strcmp (name, symbol->name))
        return symbol;
    }

  return NULL;
}