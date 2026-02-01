#ifndef LOOM_COMMAND_H
#define LOOM_COMMAND_H 1

#include "loom/compiler.h"
#include "loom/types.h"

struct loom_command_t;

typedef int (*loom_task_t) (struct loom_command_t *, loom_usize_t, char *[]);

typedef struct loom_command_t
{
  loom_task_t task;
  const char *name;
  void *data;
  struct loom_command_t *prev, *next;
} loom_command_t;

extern loom_command_t *loom_commands;

void LOOM_EXPORT (loom_command_register) (loom_command_t *command);
void LOOM_EXPORT (loom_command_unregister) (loom_command_t *command);

loom_command_t *LOOM_EXPORT (loom_command_find) (const char *name);

#endif