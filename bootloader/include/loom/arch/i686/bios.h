#ifndef LOOM_I686_BIOS_H
#define LOOM_I686_BIOS_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
  loom_uint32_t eax;
  loom_uint32_t ebx;
  loom_uint32_t ecx;
  loom_uint32_t edx;
  loom_uint32_t esi;
  loom_uint32_t edi;
  loom_uint16_t flags;
  loom_uint16_t ds;
  loom_uint16_t es;
} PACKED loom_bios_args_t;

void EXPORT (loom_enter_rmode) (void);
void EXPORT (loom_bios_int) (loom_uint8_t intno, loom_bios_args_t *args);

void loom_bios_disk_probe (void);

#endif