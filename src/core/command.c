#include "loom/command.h"
#include "loom/assert.h"
#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/string.h"
#include "loom/types.h"

loom_list loom_commands = LOOM_LIST_HEAD (loom_commands);

void
loomCommandRegister (loom_command *command)
{
  loomAssert (command != NULL);
  loomListAppend (&loom_commands, &command->node);
}

void
loomCommandUnregister (loom_command *command)
{
  loomAssert (command != NULL);
  loomListRemove (&command->node);
}

loom_command *
loomCommandFind (const char *name)
{
  loom_command *command;

  loom_list_for_each_entry (&loom_commands, command, node)
  {
    if (!loomStrCmp (command->name, name))
      return command;
  }

  return NULL;
}