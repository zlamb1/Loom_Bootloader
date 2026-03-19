#ifndef LOOM_I686_BIOS_H
#define LOOM_I686_BIOS_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
  u32 eax;
  u32 ebx;
  u32 ecx;
  u32 edx;
  u32 esi;
  u32 edi;
  u16 flags;
  u16 ds;
  u16 es;
} LOOM_PACKED loom_bios_args;

void LOOM_EXPORT (loom_enter_rmode) (void);
void LOOM_EXPORT (loom_bios_int) (u8 intno, loom_bios_args *args);

void loom_bios_disk_probe (void);

#endif