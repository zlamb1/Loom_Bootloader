#include "loom/main.h"
#include "loom/arch.h"
#include "loom/shell.h"

void
loom_main (void)
{
  loom_arch_init ();
  loom_exec_shell ();

  for (;;)
    ;
}