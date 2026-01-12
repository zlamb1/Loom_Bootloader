#include "loom/arch/i686/isr.h"
#include "loom/error.h"

void
loom_exception_handler (loom_uint32_t intno, loom_uint32_t error_code)
{
  (void) intno;
  (void) error_code;
  volatile int dummy;

  __asm__ volatile ("mov $1, %0" : "=m"(dummy)::"memory");

  if (dummy)
    loom_panic ("exception occurred");

  __asm__ volatile ("hlt" ::: "memory");
  for (;;)
    ;
}