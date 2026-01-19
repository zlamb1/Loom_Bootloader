#include "loom/command.h"
#include "loom/module.h"
#include "loom/print.h"

LOOM_MOD (hello)

static void
command_hello (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
               UNUSED char *argv[])
{
  loom_printf ("Hello!\n");
}

static loom_command_t hello_command = {
  .name = "hello",
  .fn = command_hello,
};

LOOM_MOD_INIT () { loom_command_register (&hello_command); }

LOOM_MOD_DEINIT () { loom_command_unregister (&hello_command); }
