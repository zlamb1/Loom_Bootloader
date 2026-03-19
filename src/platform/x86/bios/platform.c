#include "loom/platform.h"
#include "loom/mm.h"
#include "loom/platform/x86/bios/bios.h"
#include "loom/platform/x86/idt.h"
#include "loom/platform/x86/pic.h"

typedef struct
{
  u64 address;
  u64 length;
  u32 type;
} LOOM_PACKED e820_t;

void loom_vga_con_register (void);

void
mmap_mm_hook (u64 addr, u64 length, loom_memory_type type,
              LOOM_UNUSED void *data)
{
  if (addr >= 0xffffffff || addr >= USIZE_MAX || type != LOOM_MEMORY_TYPE_FREE)
    return;

  if (addr < 0x100000)
    {
      if (addr + length <= 0x100000)
        return;
      length -= 0x100000 - addr;
      addr = 0x100000;
    }

  loom_mm_add_region ((usize) addr, (usize) length);
}

void
loom_platform_init (void)
{
  // Note: Save BIOS PIC mask state before any BIOS interrupt calls.
  loom_pic_bios_save_masks ();
  loom_vga_con_register ();
  loom_platform_mmap_iterate (mmap_mm_hook, NULL);
  loom_bios_disk_probe ();
  loom_pic_remap (0x20, 0x28);
  loom_pic_disable ();
  loom_idt_init ();
  loom_idtr_load ();
  loom_sti ();
}

void
loom_platform_mmap_iterate (mmap_hook hook, void *data)
{
  loom_bios_args args = { 0 };
  volatile e820_t e820;

  loom_compile_assert (sizeof (e820_t) >= 20,
                       "E820 struct must be at least 20 bytes.");

  while (1)
    {
      args.eax = 0xE820;
      args.ecx = sizeof (e820_t);
      args.edx = 0x534D4150;
      args.edi = (uintptr) &e820;
      args.ds = 0;

      loom_bios_int (0x15, &args);

      if (args.flags & 1 || args.eax != 0x534D4150 || !args.ebx
          || args.ecx < 20)
        break;

      loom_memory_type type;

      switch (e820.type)
        {
        case 1:
          type = LOOM_MEMORY_TYPE_FREE;
          break;
        case 3:
          type = LOOM_MEMORY_TYPE_ACPI;
          break;
        case 4:
          type = LOOM_MEMORY_TYPE_ACPI_NVS;
          break;
        case 5:
          type = LOOM_MEMORY_TYPE_BAD_RAM;
          break;
        default:
          type = LOOM_MEMORY_TYPE_RESERVED;
          break;
        }

      hook (e820.address, e820.length, type, data);
    }
}

void
loom_sti (void)
{
  __asm__ volatile ("sti" ::: "memory");
}

void
loom_cli (void)
{
  __asm__ volatile ("cli" ::: "memory");
}

int
loom_irq_save (void)
{
  int flags;
  __asm__ volatile ("pushf; pop %0; cli" : "=r"(flags)::"memory");
  return flags;
}

void
loom_irq_restore (int flags)
{
  if (flags & 0x200)
    loom_sti ();
}