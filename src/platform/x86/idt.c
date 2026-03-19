#include "loom/platform/x86/idt.h"

#define SIZE 256

typedef struct
{
  u16 offset_lo;
  u16 segment;
  u8 reserved;
  u8 flags;
  u16 offset_hi;
} packed idt_entry;

typedef struct
{
  u16 size;
  u32 offset;
} packed idtr;

idt_entry idt[SIZE];
idtr _idtr;

extern void *loom_vectors[SIZE];

extern void loomExceptionHandler (u32 intno, u32 error_code);

void
loomIdtInit (void)
{
  for (int i = 0; i < SIZE; ++i)
    {
      // The isr stubs are initially stored in loom_vectors.
      loomIdtVectorMap ((u8) i, loom_vectors[i]);
      loomIsrVectorMap ((u8) i, NULL);
    }

  for (u8 i = 0; i < 32; ++i)
    loomIsrVectorMap (i, loomExceptionHandler);
}

void
loomIdtrLoad (void)
{
  _idtr.size = sizeof (idt) - 1;
  _idtr.offset = (uintptr) idt;
  __asm__ volatile ("lidt %0" ::"m"(_idtr));
}

void
loomIsrVectorMap (u8 entry, void *isr)
{
  loom_vectors[entry] = isr;
}

void
loomIdtVectorMap (u8 entry_nr, void *isr)
{
  idt_entry entry = { 0 };
  uintptr addr;

  addr = (uintptr) isr;

  entry.offset_lo = (u16) addr;
  entry.segment = 0x8;
  entry.flags = 0b10001110;
  entry.offset_hi = (u16) (addr >> 16);

  idt[entry_nr] = entry;
}

void
loomIdtVectorUnmap (u8 entry_nr)
{
  idt[entry_nr] = (idt_entry) { 0 };
}