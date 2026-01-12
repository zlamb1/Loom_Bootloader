#include "loom/arch/i686/idt.h"

#define SIZE 256

typedef struct
{
  loom_uint16_t offset_lo;
  loom_uint16_t segment;
  loom_uint8_t reserved;
  loom_uint8_t flags;
  loom_uint16_t offset_hi;
} PACKED idt_entry_t;

typedef struct
{
  loom_uint16_t size;
  loom_uint32_t offset;
} PACKED idtr_t;

idt_entry_t idt[SIZE];
idtr_t idtr;

extern void *loom_vectors[SIZE];

extern void loom_exception_handler (loom_uint32_t intno,
                                    loom_uint32_t error_code);

void
loom_idt_init (void)
{
  for (int i = 0; i < SIZE; ++i)
    {
      // The isr stubs are initially stored in loom_vectors.
      loom_map_idt_vector ((loom_uint8_t) i, loom_vectors[i]);
      loom_map_isr_vector ((loom_uint8_t) i, NULL);
    }

  for (loom_uint8_t i = 0; i < 32; ++i)
    loom_map_isr_vector (i, loom_exception_handler);
}

void
loom_load_idtr (void)
{
  idtr.size = sizeof (idt) - 1;
  idtr.offset = (loom_uintptr_t) idt;
  __asm__ volatile ("lidt %0" ::"m"(idtr));
}

void
loom_map_isr_vector (loom_uint8_t entry, void *isr)
{
  loom_vectors[entry] = isr;
}

void
loom_map_idt_vector (loom_uint8_t entry, void *isr)
{
  idt_entry_t idt_entry = { 0 };
  loom_uintptr_t addr;

  addr = (loom_uintptr_t) isr;

  idt_entry.offset_lo = (loom_uint16_t) addr;
  idt_entry.segment = 0x8;
  idt_entry.flags = 0b10001110;
  idt_entry.offset_hi = (loom_uint16_t) (addr >> 16);

  idt[entry] = idt_entry;
}