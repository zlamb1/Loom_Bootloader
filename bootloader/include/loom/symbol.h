#ifndef LOOM_SYMBOL_H
#define LOOM_SYMBOL_H 1

#include "compiler.h"

typedef struct
{
  const char *name;
  void *p;
} loom_symbol_t;

#define CSYMTAB 64

// This function's source is generated at compile time
// by gensym.sh.
void loom_register_export_symbols (void);

int EXPORT (loom_register_symbol) (const char *, void *);

loom_symbol_t *EXPORT (loom_find_symbol) (void *);

#endif