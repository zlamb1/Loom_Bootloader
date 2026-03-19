#include "loom/console.h"
#include "loom/print.h"
#include "loom/symbol.h"

extern char stage2s;
extern char stage3e;

void
loomPanic (const char *fmt, ...)
{
  static int count = 0;
  va_list args;

  if (count++ >= 1)
    for (;;)
      ;

  loomConsolesClear ();

  loomLogLn ("!!!PANIC!!!");
  va_start (args, fmt);
  loomLogV (fmt, args);
  va_end (args);

  void **ebp = (void **) __builtin_frame_address (0);

  loomLogLn ("\n\nBacktrace:");

  while (*ebp)
    {
      loom_symbol *symbol;
      void **next = ebp[0];
      void *addr = ebp[1];

      if ((uintptr_t) addr < (uintptr_t) &stage2s
          || (uintptr_t) addr > (uintptr_t) &stage3e
          || (uintptr_t) ebp > (uintptr_t) next)
        break;

      symbol = loomSymbolFind (addr);
      loomLogLn ("%s() @ %p", symbol ? symbol->name : "???", addr);

      ebp = next;
    }

  for (;;)
    ;
}