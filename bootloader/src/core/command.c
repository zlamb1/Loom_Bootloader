#include "loom/command.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_command_t *loom_commands = NULL;

void
loom_command_register (loom_command_t *command)
{
  command->next = loom_commands;
  loom_commands = command;
}

loom_command_t *
loom_command_find (const char *name)
{
  LOOM_LIST_ITERATE (loom_commands, command)
  {
    if (loom_streq (command->name, name))
      return command;
  }

  return NULL;
}