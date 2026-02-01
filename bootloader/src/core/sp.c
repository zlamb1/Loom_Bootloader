#include <stdint.h>

#include "loom/error.h"
#include "loom/sp.h"

uintptr_t __stack_chk_guard = 0xAB9EF4C2;

void LOOM_NOINLINE
__stack_chk_fail (void)
{
  volatile int dummy = 1;
  if (dummy)
    loom_panic ("stack smashing detected");
  for (;;)
    ;
}