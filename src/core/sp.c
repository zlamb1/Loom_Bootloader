#include "loom/sp.h"
#include "loom/error.h"

uintptr_t __stack_chk_guard = 0xAB9EF4C2;

void noinline
__stack_chk_fail (void)
{
  volatile int dummy = 1;
  if (dummy)
    loomPanic ("stack smashing detected");
  for (;;)
    ;
}