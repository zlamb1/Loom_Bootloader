#ifndef LOOM_ARCH_H
#define LOOM_ARCH_H 1

#include "loom/compiler.h"
#include "loom/mmap.h"

extern char LOOM_EXPORT_VAR (stage1s);
extern char LOOM_EXPORT_VAR (stage3e);

typedef void (*mmap_hook) (loom_uint64_t, loom_uint64_t, loom_memory_type_t,
                           void *);

void LOOM_EXPORT (loom_arch_init) (void);
void LOOM_EXPORT (loom_arch_mmap_iterate) (mmap_hook hook, void *);

void LOOM_EXPORT (loom_arch_sti) (void);
void LOOM_EXPORT (loom_arch_cli) (void);

int LOOM_EXPORT (loom_arch_irq_save) (void);
void LOOM_EXPORT (loom_arch_irq_restore) (int);

void LOOM_NORETURN LOOM_EXPORT (loom_arch_reboot) (void);

#endif