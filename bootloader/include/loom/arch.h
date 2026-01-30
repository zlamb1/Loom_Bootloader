#ifndef LOOM_ARCH_H
#define LOOM_ARCH_H 1

#include "compiler.h"
#include "mmap.h"

extern char EXPORT_VAR (stage1s);
extern char EXPORT_VAR (stage3e);

typedef void (*mmap_hook) (loom_uint64_t, loom_uint64_t, loom_memory_type_t,
                           void *);

void EXPORT (loom_arch_init) (void);
void EXPORT (loom_arch_mmap_iterate) (mmap_hook hook, void *);

void EXPORT (loom_arch_sti) (void);
void EXPORT (loom_arch_cli) (void);

int EXPORT (loom_arch_irq_save) (void);
void EXPORT (loom_arch_irq_restore) (int);

void NORETURN EXPORT (loom_arch_reboot) (void);

#endif