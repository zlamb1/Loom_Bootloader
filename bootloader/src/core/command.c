#include "loom/command.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_command_t *loom_commands = NULL;

void
loom_command_register (loom_command_t *command)
{
  command->prev = NULL;
  command->next = loom_commands;

  if (loom_commands)
    loom_commands->prev = command;

  loom_commands = command;
}

void
loom_command_unregister (loom_command_t *command)
{
  if (command->prev)
    command->prev->next = command->next;

  if (command->next)
    command->next->prev = command->prev;

  if (command == loom_commands)
    loom_commands = command->next;

  command->prev = NULL;
  command->next = NULL;
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