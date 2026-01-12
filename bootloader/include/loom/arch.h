#ifndef LOOM_ARCH_H
#define LOOM_ARCH_H 1

#include "compiler.h"
#include "mmap.h"

typedef void (*mmap_hook) (loom_uint64_t, loom_uint64_t, loom_memory_type_t);

void EXPORT (loom_arch_init) (void);
void EXPORT (loom_arch_mmap_iterate) (mmap_hook hook);

#endif