#ifndef LOOM_LOADER_H
#define LOOM_LOADER_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct loom_kernel_module
{
  void *data;
  struct loom_kernel_module *next;
} loom_kernel_module;

typedef struct loom_kernel_loader
{
  void (*boot) (struct loom_kernel_loader *kernel_loader);

#define LOOM_KERNEL_LOADER_FLAG_MODULES (1 << 0)
#define LOOM_KERNEL_LOADER_FLAG_INITRD  (1 << 1)
  unsigned int flags;
  usize kernel_size;
  void *kernel;
  void *initrd;
  loom_kernel_module *modules;
  void *data;
} loom_kernel_loader;

void LOOM_EXPORT (loom_kernel_loader_add) (loom_kernel_loader *kernel_loader);
int LOOM_EXPORT (loom_kernel_loader_remove) (bool free);

int LOOM_EXPORT (loom_kernel_loader_boot) (void);

#endif