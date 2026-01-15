#include "loom/types.h"
#ifndef LOOM_SYMBOL_H
#define LOOM_SYMBOL_H 1

#include "compiler.h"

typedef struct
{
  const char *name;
  void *p;
} loom_symbol_t;

typedef struct
{
  loom_usize_t length;
#define LOOM_SYMTAB_SIZE 128
  loom_symbol_t *symbols[LOOM_SYMTAB_SIZE];
} loom_symtab_t;

extern loom_symtab_t loom_symtab;

// This function's source is generated at compile time by gensym.sh.
void loom_register_export_symbols (void);

int EXPORT (loom_symbol_register) (const char *, void *);

loom_symbol_t *EXPORT (loom_symbol_find) (void *);

#endif