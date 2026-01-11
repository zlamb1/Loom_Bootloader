#ifndef LOOM_I686_BIOS_H
#define LOOM_I686_BIOS_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
  loom_u32 eax;
  loom_u32 ebx;
  loom_u32 ecx;
  loom_u32 edx;
  loom_u32 esi;
  loom_u32 edi;
  loom_u16 flags;
  loom_u16 ds;
  loom_u16 es;
} PACKED loom_bios_args_t;

#endif