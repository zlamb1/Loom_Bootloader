#include "loom/command.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_command_t *commands = NULL;

void
loom_register_command (loom_command_t *command)
{
  command->next = commands;
  commands = command;
}

loom_command_t *
loom_find_command (const char *name)
{
  LOOM_LIST_ITERATE (commands, command)
  {
    if (loom_streq (command->name, name))
      return command;
  }

  return NULL;
}