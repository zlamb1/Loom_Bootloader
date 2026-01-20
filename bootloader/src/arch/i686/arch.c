#include "loom/arch.h"
#include "loom/arch/i686/bios.h"
#include "loom/arch/i686/idt.h"
#include "loom/arch/i686/pic.h"
#include "loom/arch/i686/ps2.h"
#include "loom/mm.h"

typedef struct
{
  loom_uint64_t address;
  loom_uint64_t length;
  loom_uint32_t type;
} PACKED e820_t;

void loom_vga_con_register (void);

void
mmap_mm_hook (loom_uint64_t address, loom_uint64_t length,
              loom_memory_type_t type)
{
  if (address >= 0xffffffff || address >= LOOM_USIZE_MAX
      || type != LOOM_MEMORY_TYPE_FREE)
    return;

  if (address < 0x100000)
    {
      if (address + length <= 0x100000)
        return;
      length -= 0x100000 - address;
      address = 0x100000;
    }

  loom_mm_add_region ((loom_usize_t) address, (loom_usize_t) length);
}

void
loom_arch_init (void)
{
  loom_vga_con_register ();
  loom_arch_mmap_iterate (mmap_mm_hook);
  loom_bios_disk_probe ();
  loom_pic_remap (0x20, 0x28);
  loom_pic_disable ();
  loom_idt_init ();
  loom_ps2_kb_init ();
  loom_idtr_load ();
  loom_arch_sti ();
}

void
loom_arch_mmap_iterate (mmap_hook hook)
{
  loom_bios_args_t args = { 0 };
  volatile e820_t e820;

  compile_assert (sizeof (e820_t) >= 20,
                  "E820 struct must be at least 20 bytes.");

  while (1)
    {
      args.eax = 0xE820;
      args.ecx = sizeof (e820_t);
      args.edx = 0x534D4150;
      args.edi = (loom_uintptr_t) &e820;
      args.ds = 0;

      loom_bios_int (0x15, &args);

      if (args.flags & 1 || args.eax != 0x534D4150 || !args.ebx
          || args.ecx < 20)
        break;

      loom_memory_type_t type;

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

      hook (e820.address, e820.length, type);
    }
}

void
loom_arch_sti (void)
{
  __asm__ volatile ("sti" ::: "memory");
}

void
loom_arch_cli (void)
{
  __asm__ volatile ("cli" ::: "memory");
}

int
loom_arch_irq_save (void)
{
  int flags;
  __asm__ volatile ("pushf; pop %0" : "=r"(flags)::"memory");
  return flags;
}

void
loom_arch_irq_restore (int flags)
{
  if (flags & 0x200)
    loom_arch_sti ();
}

void
loom_arch_reboot (void)
{
  // Unmap GPF and DF handlers so we triple fault.
  loom_idt_vector_unmap (8);
  loom_idt_vector_unmap (13);

  __asm__ volatile ("cli; ljmp $0xFFFF, $0");
loop:
  __asm__ volatile ("hlt");
  goto loop;
}