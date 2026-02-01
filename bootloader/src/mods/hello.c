#include "loom/command.h"
#include "loom/module.h"
#include "loom/print.h"

LOOM_MOD (hello)

static int
hello_task (LOOM_UNUSED loom_command_t *cmd, LOOM_UNUSED loom_usize_t argc,
            LOOM_UNUSED char *argv[])
{
  loom_printf ("Hello!\n");
  return 0;
}

static loom_command_t hello_command = {
  .name = "hello",
  .task = hello_task,
};

LOOM_MOD_INIT () { loom_command_register (&hello_command); }

LOOM_MOD_DEINIT () { loom_command_unregister (&hello_command); }
