#include "loom/arch.h"
#include "loom/arch/i686/bios.h"

typedef struct
{
  loom_uint64_t address;
  loom_uint64_t length;
  loom_uint32_t type;
} PACKED e820_t;

void loom_vga_con_register (void);

void
loom_arch_init (void)
{
  loom_vga_con_register ();
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