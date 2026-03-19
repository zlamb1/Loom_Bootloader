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

extern void loom_exception_handler (u32 intno, u32 error_code);

void
loom_idt_init (void)
{
  for (int i = 0; i < SIZE; ++i)
    {
      // The isr stubs are initially stored in loom_vectors.
      loom_idt_vector_map ((u8) i, loom_vectors[i]);
      loom_isr_vector_map ((u8) i, NULL);
    }

  for (u8 i = 0; i < 32; ++i)
    loom_isr_vector_map (i, loom_exception_handler);
}

void
loom_idtr_load (void)
{
  _idtr.size = sizeof (idt) - 1;
  _idtr.offset = (uintptr) idt;
  __asm__ volatile ("lidt %0" ::"m"(_idtr));
}

void
loom_isr_vector_map (u8 entry, void *isr)
{
  loom_vectors[entry] = isr;
}

void
loom_idt_vector_map (u8 entry_nr, void *isr)
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
loom_idt_vector_unmap (u8 entry_nr)
{
  idt[entry_nr] = (idt_entry) { 0 };
}