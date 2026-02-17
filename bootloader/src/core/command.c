#include "loom/command.h"
#include "loom/assert.h"
#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_list_t loom_commands = LOOM_LIST_HEAD (loom_commands);

void
loom_command_register (loom_command_t *command)
{
  loom_assert (command != NULL);
  loom_list_append (&loom_commands, &command->node);
}

void
loom_command_unregister (loom_command_t *command)
{
  loom_assert (command != NULL);
  loom_list_remove (&command->node);
}

loom_command_t *
loom_command_find (const char *name)
{
  loom_command_t *command;

  loom_list_for_each_entry (&loom_commands, command, node)
  {
    if (!loom_strcmp (command->name, name))
      return command;
  }

  return NULL;
}