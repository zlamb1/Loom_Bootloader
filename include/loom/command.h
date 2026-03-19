#ifndef LOOM_COMMAND_H
#define LOOM_COMMAND_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

struct loom_command;

typedef int (*loom_task) (struct loom_command *, usize, char *[]);

typedef struct loom_command
{
  loom_task task;
  const char *name;
  void *data;
  loom_list node;
} loom_command;

extern loom_list loom_commands;

void LOOM_EXPORT (loom_command_register) (loom_command *command);
void LOOM_EXPORT (loom_command_unregister) (loom_command *command);

loom_command *LOOM_EXPORT (loom_command_find) (const char *name);

#endif