#ifndef LOOM_ARCH_H
#define LOOM_ARCH_H 1

#include "loom/compiler.h"
#include "loom/mmap.h"

extern char export_var (stage1s);
extern char export_var (stage3e);

typedef void (*mmap_hook) (u64, u64, loom_memory_type, void *);

void export (loomPlatformInit) (void);
void export (loomPlatformMmapIterate) (mmap_hook hook, void *);

void export (loomSti) (void);
void export (loomCli) (void);

int export (loomIrqSave) (void);
void export (loomIrqRestore) (int);

void noreturn export (loomReboot) (void);

#endif