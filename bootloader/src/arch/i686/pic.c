#include "loom/arch/i686/pic.h"
#include "loom/arch.h"
#include "loom/arch/i686/idt.h"
#include "loom/arch/i686/io.h"
#include "loom/error.h"

#define PIC_EOI 0x20

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define ICW4_8086 0x01

#define CASCADE_IRQ 0x02

#define BIOS_MASTER_OFFSET 0x8
#define BIOS_SLAVE_OFFSET  0x70

#define DEFAULT_MASK 0xFF

static loom_uint8_t offsets[2] = { BIOS_MASTER_OFFSET, BIOS_SLAVE_OFFSET };
static loom_uint8_t masks[2] = { DEFAULT_MASK, DEFAULT_MASK }, bios_masks[2];

static void
pic_remap (loom_uint8_t offset1, loom_uint8_t offset2, loom_bool_t save)
{
  int flags = loom_arch_irq_save ();

  if (offset1 % 8 != 0 || offset2 % 8 != 0)
    loom_panic ("loom_pic_remap: bad offset");

  loom_outb (PIC1_CMD, ICW1_ICW4 | ICW1_INIT);
  loom_outb (PIC2_CMD, ICW1_ICW4 | ICW1_INIT);
  loom_outb (PIC1_DATA, offset1);
  loom_outb (PIC2_DATA, offset2);
  loom_outb (PIC1_DATA, 1 << CASCADE_IRQ);
  loom_outb (PIC2_DATA, CASCADE_IRQ);
  loom_outb (PIC1_DATA, ICW4_8086);
  loom_outb (PIC2_DATA, ICW4_8086);
  loom_outb (PIC1_DATA, DEFAULT_MASK);
  loom_outb (PIC2_DATA, DEFAULT_MASK);

  if (save)
    {
      offsets[0] = offset1;
      offsets[1] = offset2;
      masks[0] = DEFAULT_MASK;
      masks[1] = DEFAULT_MASK;
    }

  loom_arch_irq_restore (flags);
}

void
loom_pic_remap (loom_uint8_t offset1, loom_uint8_t offset2)
{
  pic_remap (offset1, offset2, 1);
}

void
loom_pic_mask (loom_uint8_t irq)
{
  loom_uint16_t port;
  loom_uint8_t mask;

  if (irq < 8)
    port = PIC1_DATA;
  else
    {
      port = PIC2_DATA;
      irq -= 8;
    }

  mask = loom_inb (port) | (uint8_t) (1 << irq);

  if (irq < 8)
    masks[0] = mask;
  else
    masks[1] = mask;

  loom_outb (port, mask);
}

void
loom_pic_unmask (loom_uint8_t irq)
{
  loom_uint16_t port;
  loom_uint8_t mask;

  if (irq < 8)
    port = PIC1_DATA;
  else
    {
      port = PIC2_DATA;
      irq -= 8;
    }

  mask = loom_inb (port) & (loom_uint8_t) ~(1 << irq);

  if (irq < 8)
    masks[0] = mask;
  else
    masks[1] = mask;

  loom_outb (port, mask);
}

void
loom_pic_eoi (loom_uint8_t irq)
{
  if (irq >= 8)
    loom_outb (PIC2_CMD, PIC_EOI);
  loom_outb (PIC1_CMD, PIC_EOI);
}

void
loom_pic_disable (void)
{
  loom_outb (PIC1_DATA, 0xFF);
  loom_outb (PIC2_DATA, 0xFF);
}

void
loom_pic_register_isr (loom_uint8_t irq, void *isr)
{
  loom_uint8_t offset;
  if (irq < 8)
    offset = offsets[0];
  else
    {
      offset = offsets[1];
      irq -= 8;
    }

  loom_isr_vector_map (offset + irq, isr);
}

void
loom_pic_bios_save_masks (void)
{
  bios_masks[0] = loom_inb (PIC1_DATA);
  bios_masks[1] = loom_inb (PIC2_DATA);
}

void
loom_pic_bios_reset (void)
{
  int flags = loom_arch_irq_save ();
  pic_remap (BIOS_MASTER_OFFSET, BIOS_SLAVE_OFFSET, 0);
  loom_outb (PIC1_DATA, bios_masks[0]);
  loom_outb (PIC2_DATA, bios_masks[1]);
  loom_arch_irq_restore (flags);
}

void
loom_pic_bios_restore (void)
{
  int flags = loom_arch_irq_save ();
  pic_remap (offsets[0], offsets[1], 0);
  loom_outb (PIC1_DATA, masks[0]);
  loom_outb (PIC2_DATA, masks[1]);
  loom_arch_irq_restore (flags);
}