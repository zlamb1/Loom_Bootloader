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
  struct loom_command_t *prev, *next;
} loom_command_t;

extern loom_command_t *loom_commands;

void EXPORT (loom_command_register) (loom_command_t *command);
void EXPORT (loom_command_unregister) (loom_command_t *command);

loom_command_t *EXPORT (loom_command_find) (const char *name);

#endif