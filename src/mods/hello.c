#include "loom/command.h"
#include "loom/module.h"
#include "loom/print.h"

LOOM_MOD (hello)

static int
hello_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loomLogLn ("Hello!");
  return 0;
}

static loom_command hello_command = {
  .name = "hello",
  .task = hello_task,
};

LOOM_MOD_INIT () { loomCommandRegister (&hello_command); }

LOOM_MOD_DEINIT () { loomCommandUnregister (&hello_command); }
