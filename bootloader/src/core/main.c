#include "loom/arch.h"
#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/print.h"

void NORETURN
loom_main (void)
{
  loom_arch_init ();
  loom_con_write (6, "Hello!");
  for (;;)
    ;
}