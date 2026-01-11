#include "loom/arch.h"
#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/print.h"
#include "loom/shell.h"

void
loom_panic (const char *msg)
{
  static int count = 0;

  if (count++ > 1)
    for (;;)
      ;

  loom_con_clear ();

  loom_con_write_str ("!!!PANIC!!!\n");
  loom_con_write_str (msg);

  for (;;)
    ;
}

void NORETURN
loom_main (void)
{
  loom_arch_init ();
  loom_printf ("Hello, Loom.\n");
  loom_exec_shell ();

  for (;;)
    ;
}