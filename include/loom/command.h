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

void export (loomCommandRegister) (loom_command *command);
void export (loomCommandUnregister) (loom_command *command);

loom_command *export (loomCommandFind) (const char *name);

#endif