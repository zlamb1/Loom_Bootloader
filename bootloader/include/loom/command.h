#ifndef LOOM_COMMAND_H
#define LOOM_COMMAND_H 1

#include "compiler.h"
#include "types.h"

struct loom_command_t;

typedef void (*loom_fn_t) (struct loom_command_t *, loom_usize_t, char *[]);

typedef struct loom_command_t
{
  loom_fn_t fn;
  const char *name;
  void *data;
  struct loom_command_t *next;
} loom_command_t;

void EXPORT (loom_register_command) (loom_command_t *command);
loom_command_t *EXPORT (loom_find_command) (const char *name);

#endif