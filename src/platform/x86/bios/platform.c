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
} packed e820_t;

void loomRegisterEarlyVgaConsole (void);

void
mmapMMHook (u64 addr, u64 length, loom_memory_type type, unused void *data)
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

  loomHeapAddRegion ((usize) addr, (usize) length);
}

void
loomPlatformInit (void)
{
  // Note: Save BIOS PIC mask state before any BIOS interrupt calls.
  loomPICSaveBiosDefaults ();
  loomRegisterEarlyVgaConsole ();
  loomPlatformMmapIterate (mmapMMHook, NULL);
  loomBiosDisksProbe ();
  loomPICRemap (0x20, 0x28);
  loomPICDisable ();
  loomIdtInit ();
  loomIdtrLoad ();
  loomSti ();
}

void
loomPlatformMmapIterate (mmap_hook hook, void *data)
{
  loom_bios_args args = { 0 };
  volatile e820_t e820;

  compile_assert (sizeof (e820_t) >= 20,
                  "E820 struct must be at least 20 bytes.");

  while (1)
    {
      args.eax = 0xE820;
      args.ecx = sizeof (e820_t);
      args.edx = 0x534D4150;
      args.edi = (uintptr) &e820;
      args.ds = 0;

      loomBiosInt (0x15, &args);

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
loomSti (void)
{
  __asm__ volatile ("sti" ::: "memory");
}

void
loomCli (void)
{
  __asm__ volatile ("cli" ::: "memory");
}

int
loomIrqSave (void)
{
  int flags;
  __asm__ volatile ("pushf; pop %0; cli" : "=r"(flags)::"memory");
  return flags;
}

void
loomIrqRestore (int flags)
{
  if (flags & 0x200)
    loomSti ();
}