#include "loom/symbol.h"
#include "loom/mm.h"
#include "loom/string.h"
#include "loom/types.h"

loom_symbol_table loom_symtab;

int
loom_symbol_register (const char *name, bool is_fn, void *p)
{
  loom_symbol *symbol;

  if (loom_symtab.length >= LOOM_SYMTAB_SIZE)
    return 0;

  symbol = loom_malloc (sizeof (loom_symbol));
  if (!symbol)
    return 0;

  symbol->name = name;
  symbol->is_fn = is_fn;
  symbol->p = p;

  loom_symtab.symbols[loom_symtab.length++] = symbol;

  return 1;
}

loom_symbol *
loom_symbol_find (void *p)
{
  loom_symbol *nearest = NULL;

  for (usize i = 0; i < loom_symtab.length; i++)
    {
      loom_symbol *symbol = loom_symtab.symbols[i];

      if (p < symbol->p)
        continue;

      if (!nearest || symbol->p > nearest->p)
        nearest = symbol;
    }

  return nearest;
}

loom_symbol *
loom_symbol_lookup (const char *name)
{
  for (unsigned int i = 0; i < loom_symtab.length; ++i)
    {
      loom_symbol *symbol = loom_symtab.symbols[i];

      if (!loom_strcmp (name, symbol->name))
        return symbol;
    }

  return NULL;
}