#include "loom/console.h"
#include "loom/print.h"

void
loom_panic (const char *msg)
{
  static int count = 0;

  if (count++ > 1)
    for (;;)
      ;

  loom_con_clear ();

  loom_printf ("!!!PANIC!!!\n");
  loom_con_write_str (msg);

  for (;;)
    ;
}