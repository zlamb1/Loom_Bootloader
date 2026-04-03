#ifndef LOOM_LOADER_H
#define LOOM_LOADER_H 1

#include "loom/buffer.h"
#include "loom/compiler.h"

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
  loom_buffer kernel;
  usize initrd_min_addr, initrd_max_addr;
  loom_buffer initrd;
  loom_buffer cmdline;
  loom_kernel_module *modules;
  void *data;
} loom_kernel_loader;

extern loom_kernel_loader *export (kernel_loader);

void export (loomKernelLoaderSet) (loom_kernel_loader *kernel_loader);
int export (loomKernelLoaderRemove) (void);
int export (loomKernelLoaderBoot) (void);

#endif