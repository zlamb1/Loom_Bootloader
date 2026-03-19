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
void loomRegisterExportSymbols (void);

int export (loomSymbolRegister) (const char *name, bool is_fn, void *p);

loom_symbol *export (loomSymbolFind) (void *p);
loom_symbol *export (loomSymbolLookup) (const char *name);

#endif