#ifndef LOOM_SYMBOL_H
#define LOOM_SYMBOL_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
  const char *name;
  loom_bool_t isfunc;
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

int LOOM_EXPORT (loom_symbol_register) (const char *name, loom_bool_t isfunc,
                                        void *p);

loom_symbol_t *LOOM_EXPORT (loom_symbol_find) (void *p);
loom_symbol_t *LOOM_EXPORT (loom_symbol_lookup) (const char *name);

#endif