#include "loom/platform/x86/io.h"

void
loomOutByte (u16 port, u8 val)
{
  __asm__ volatile ("outb %1, %0" ::"Nd"(port), "a"(val));
}

u8
loomInByte (u16 port)
{
  u8 out;
  __asm__ volatile ("inb %1, %0" : "=a"(out) : "Nd"(port));
  return out;
}
