#include "loom/main.h"
#include "loom/arch.h"
#include "loom/shell.h"
#include "loom/symbol.h"

extern char stage1s;

void
loom_main (void)
{
  loom_arch_init ();
  loom_register_export_symbols ();

  //__asm__ volatile ("mov $0, %edx; div %edx");

  loom_exec_shell ();

  for (;;)
    ;
}