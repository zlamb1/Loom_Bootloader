#include "loom/console.h"
#include "loom/print.h"
#include "loom/symbol.h"

extern char stage2s;
extern char stage3e;

void
loom_panic (const char *msg)
{
  static int count = 0;

  if (count++ >= 1)
    for (;;)
      ;

  loom_con_clear ();

  loom_printf ("!!!PANIC!!!\n");
  loom_con_write_str (msg);

  void **ebp = (void **) __builtin_frame_address (0);

  loom_printf ("\n\nBacktrace:\n");

  while (*ebp)
    {
      loom_symbol_t *symbol;
      void **next = ebp[0];
      void *addr = ebp[1];

      if ((uintptr_t) addr < (uintptr_t) &stage2s
          || (uintptr_t) addr > (uintptr_t) &stage3e
          || (uintptr_t) ebp > (uintptr_t) next)
        break;

      symbol = loom_find_symbol (addr);
      loom_printf ("%s() @ %p\n", symbol ? symbol->name : "???", addr);

      ebp = next;
    }

  for (;;)
    ;
}