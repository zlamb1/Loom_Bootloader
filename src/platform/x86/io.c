#include "loom/platform/x86/io.h"

void
loom_outb (loom_uint16_t port, loom_uint8_t val)
{
  __asm__ volatile ("outb %1, %0" ::"Nd"(port), "a"(val));
}

loom_uint8_t
loom_inb (loom_uint16_t port)
{
  loom_uint8_t out;
  __asm__ volatile ("inb %1, %0" : "=a"(out) : "Nd"(port));
  return out;
}
