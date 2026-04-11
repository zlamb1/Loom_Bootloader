#ifndef LOOM_I686_BIOS_H
#define LOOM_I686_BIOS_H 1

#define BIOS_IO_REQS_LIMIT 64

#ifndef __ASM__

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
} packed loom_bios_args;

typedef struct loom_bios_hook
{
#define LOOM_BIOS_HOOK_TYPE_ENTER 0
#define LOOM_BIOS_HOOK_TYPE_LEAVE 1
  void (*fn) (uint type, void *ctx);
  void *ctx;
  struct loom_bios_hook *next;
} loom_bios_hook;

void export (loomEnterRealMode) (void);
void export (loomEnterProtectedMode) (void);

void export (loomBiosHookRegister) (loom_bios_hook *hook);
void export (loomBiosHookUnregister) (loom_bios_hook *hook);

void export (loomRunBiosEnterHooks) (void);
void export (loomRunBiosLeaveHooks) (void);

void export (loomBiosInt) (u8 intno, loom_bios_args *args);

void loomBiosDisksProbe (void);

#endif

#endif