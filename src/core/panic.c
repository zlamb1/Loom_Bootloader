#include "loom/console.h"
#include "loom/print.h"
#include "loom/symbol.h"

extern char _sboot;
extern char _eboot;

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

      if ((uintptr) addr < (uintptr) &_sboot
          || (uintptr) addr > (uintptr) &_eboot
          || (uintptr) ebp > (uintptr) next)
        break;

      symbol = loomSymbolFind (addr);
      loomLogLn ("%s() @ %p", symbol ? symbol->name : "???", addr);

      ebp = next;
    }

  for (;;)
    ;
}