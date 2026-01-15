#include "loom/command.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_command_t *loom_command_list = NULL;

void
loom_register_command (loom_command_t *command)
{
  command->next = loom_command_list;
  loom_command_list = command;
}

loom_command_t *
loom_find_command (const char *name)
{
  LOOM_LIST_ITERATE (loom_command_list, command)
  {
    if (loom_streq (command->name, name))
      return command;
  }

  return NULL;
}