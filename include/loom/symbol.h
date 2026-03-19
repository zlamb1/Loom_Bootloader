#ifndef LOOM_SYMBOL_H
#define LOOM_SYMBOL_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
  const char *name;
  bool is_fn;
  void *p;
} loom_symbol;

typedef struct
{
  usize length;
#define LOOM_SYMTAB_SIZE 128
  loom_symbol *symbols[LOOM_SYMTAB_SIZE];
} loom_symbol_table;

extern loom_symbol_table loom_symtab;

// This function's source is generated at compile time by gensym.sh.
void loom_register_export_symbols (void);

int LOOM_EXPORT (loom_symbol_register) (const char *name, bool is_fn, void *p);

loom_symbol *LOOM_EXPORT (loom_symbol_find) (void *p);
loom_symbol *LOOM_EXPORT (loom_symbol_lookup) (const char *name);

#endif