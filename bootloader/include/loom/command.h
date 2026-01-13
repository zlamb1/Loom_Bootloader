#ifndef LOOM_COMMAND_H
#define LOOM_COMMAND_H 1

#include "compiler.h"

typedef struct loom_command_t
{
  void (*exec) (struct loom_command_t *);
  const char *name;
  void *data;
  struct loom_command_t *next;
} loom_command_t;

void EXPORT (loom_command_register) (loom_command_t *command);
loom_command_t *EXPORT (loom_command_find) (const char *name);

#endif