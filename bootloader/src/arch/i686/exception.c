#include "loom/arch/i686/isr.h"
#include "loom/error.h"

const char *exc_messages[31] = {
  [0] = "division error",
  [1] = "debug",
  [2] = "non-maskable interrupt",
  [3] = "breakpoint",
  [4] = "overflow",
  [5] = "bound range exceeded",
  [6] = "invalid opcode",
  [7] = "device not available",
  [8] = "double fault",
  [10] = "invalid TSS",
  [11] = "segment not present",
  [12] = "stack-segment fault",
  [13] = "general protection fault",
  [14] = "page fault",
  [16] = "x87 floating-point exception",
  [17] = "alignment check",
  [18] = "machine check",
  [19] = "SIMD floating-point exception",
  [20] = "virtualization exception",
  [21] = "control protection exception",
  [28] = "hypervisor injection exception",
  [29] = "VMM communication exception",
  [30] = "security exception",
};

void
loom_exception_handler (loom_uint32_t intno, loom_uint32_t error_code)
{
  (void) intno;
  (void) error_code;
  volatile int dummy;

  __asm__ volatile ("movl $1, %0" : "=m"(dummy)::"memory");

  const char *exc_message
      = intno < (sizeof (exc_messages) / sizeof (*exc_messages))
            ? exc_messages[intno]
            : NULL;

  if (!exc_message)
    exc_message = "unknown";

  if (dummy)
    loom_panic ("exception occurred: %s", exc_message);

  __asm__ volatile ("hlt" ::: "memory");
  for (;;)
    ;
}