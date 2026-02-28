#include "loom/main.h"
#include "loom/commands/core.h"
#include "loom/module.h"
#include "loom/platform.h"
#include "loom/shell.h"
#include "loom/symbol.h"

void
loom_main (void)
{
  loom_platform_init ();
  loom_mmap_init ();
  loom_register_export_symbols ();
  loom_init_core_cmds ();
  loom_core_modules_load ();
  loom_shell_exec ();
  for (;;)
    ;
}