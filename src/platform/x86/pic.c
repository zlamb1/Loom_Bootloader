#include "loom/platform/x86/pic.h"
#include "loom/error.h"
#include "loom/platform.h"
#include "loom/platform/x86/bios/bios.h"
#include "loom/platform/x86/idt.h"
#include "loom/platform/x86/io.h"

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

static u8 offsets[2] = { BIOS_MASTER_OFFSET, BIOS_SLAVE_OFFSET };
static u8 masks[2] = { DEFAULT_MASK, DEFAULT_MASK }, bios_masks[2];

static void
picRemap (u8 offset1, u8 offset2, bool save)
{
  int flags = loomIrqSave ();

  if (offset1 % 8 != 0 || offset2 % 8 != 0)
    loomPanic ("picRemap: bad offset");

  loomOutByte (PIC1_CMD, ICW1_ICW4 | ICW1_INIT);
  loomOutByte (PIC2_CMD, ICW1_ICW4 | ICW1_INIT);
  loomOutByte (PIC1_DATA, offset1);
  loomOutByte (PIC2_DATA, offset2);
  loomOutByte (PIC1_DATA, 1 << CASCADE_IRQ);
  loomOutByte (PIC2_DATA, CASCADE_IRQ);
  loomOutByte (PIC1_DATA, ICW4_8086);
  loomOutByte (PIC2_DATA, ICW4_8086);
  loomOutByte (PIC1_DATA, DEFAULT_MASK);
  loomOutByte (PIC2_DATA, DEFAULT_MASK);

  if (save)
    {
      offsets[0] = offset1;
      offsets[1] = offset2;
      masks[0] = DEFAULT_MASK;
      masks[1] = DEFAULT_MASK;
    }

  loomIrqRestore (flags);
}

void
loomPICRemap (u8 offset1, u8 offset2)
{
  picRemap (offset1, offset2, 1);
}

void
loomPICMask (u8 irq)
{
  u16 port;
  u8 mask;

  if (irq < 8)
    port = PIC1_DATA;
  else
    {
      port = PIC2_DATA;
      irq -= 8;
    }

  mask = loomInByte (port) | (uint8_t) (1 << irq);

  if (irq < 8)
    masks[0] = mask;
  else
    masks[1] = mask;

  loomOutByte (port, mask);
}

void
loomPICUnmask (u8 irq)
{
  u16 port;
  u8 mask;

  if (irq < 8)
    port = PIC1_DATA;
  else
    {
      port = PIC2_DATA;
      irq -= 8;
    }

  mask = loomInByte (port) & (u8) ~(1 << irq);

  if (irq < 8)
    masks[0] = mask;
  else
    masks[1] = mask;

  loomOutByte (port, mask);
}

void
loomPICAckIrq (u8 irq)
{
  if (irq >= 8)
    loomOutByte (PIC2_CMD, PIC_EOI);
  loomOutByte (PIC1_CMD, PIC_EOI);
}

void
loomPICDisable (void)
{
  loomOutByte (PIC1_DATA, 0xFF);
  loomOutByte (PIC2_DATA, 0xFF);
}

void
loomPICRegisterIsr (u8 irq, void *isr)
{
  u8 offset;
  if (irq < 8)
    offset = offsets[0];
  else
    {
      offset = offsets[1];
      irq -= 8;
    }

  loomIsrVectorMap (offset + irq, isr);
}

void
loomPICSaveBiosDefaults (void)
{
  bios_masks[0] = loomInByte (PIC1_DATA);
  bios_masks[1] = loomInByte (PIC2_DATA);
}

void
loomPICResetBiosDefaults (void)
{
  int flags = loomIrqSave ();
  picRemap (BIOS_MASTER_OFFSET, BIOS_SLAVE_OFFSET, 0);
  loomOutByte (PIC1_DATA, bios_masks[0]);
  loomOutByte (PIC2_DATA, bios_masks[1]);
  loomIrqRestore (flags);
}

void
loomPICRestoreMasks (void)
{
  int flags = loomIrqSave ();
  picRemap (offsets[0], offsets[1], 0);
  loomOutByte (PIC1_DATA, masks[0]);
  loomOutByte (PIC2_DATA, masks[1]);
  loomIrqRestore (flags);
}

static void
loomPICBiosHookFn (uint type, unused void *ctx)
{
  if (type == LOOM_BIOS_HOOK_TYPE_ENTER)
    loomPICResetBiosDefaults ();
  else
    {
      loomPICSaveBiosDefaults ();
      loomPICRestoreMasks ();
    }
}

loom_bios_hook loomPICBiosHook = {
  .fn = loomPICBiosHookFn,
};