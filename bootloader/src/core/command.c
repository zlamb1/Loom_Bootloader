#include "loom/command.h"
#include "loom/string.h"
#include "loom/types.h"

loom_command_t *commands = NULL;

void
loom_command_register (loom_command_t *command)
{
  command->next = commands;
  commands = command;
}

loom_command_t *
loom_command_find (const char *name)
{
  loom_command_t *command = commands;

  while (command)
    {
      if (loom_streq (command->name, name))
        return command;

      command = command->next;
    }

  return NULL;
}