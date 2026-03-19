#ifndef LOOM_ARCH_H
#define LOOM_ARCH_H 1

#include "loom/compiler.h"
#include "loom/mmap.h"

extern char export_var (stage1s);
extern char export_var (stage3e);

typedef void (*mmap_hook) (u64, u64, loom_memory_type, void *);

void export (loom_platform_init) (void);
void export (loom_platform_mmap_iterate) (mmap_hook hook, void *);

void export (loom_sti) (void);
void export (loom_cli) (void);

int export (loom_irq_save) (void);
void export (loom_irq_restore) (int);

void noreturn export (loom_reboot) (void);

#endif