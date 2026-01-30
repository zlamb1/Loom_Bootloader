#include "loom/types.h"
#ifndef LOOM_LOADER_H
#define LOOM_LOADER_H 1

#include "loom/compiler.h"

typedef struct loom_kernel_module_t
{
  void *data;
  struct loom_kernel_module_t *next;
} loom_kernel_module_t;

typedef struct loom_kernel_loader_t
{
  void (*boot) (struct loom_kernel_loader_t *kernel_loader);

#define LOOM_KERNEL_LOADER_FLAG_MODULES (1 << 0)
#define LOOM_KERNEL_LOADER_FLAG_INITRD  (1 << 1)
  unsigned int flags;
  void *kernel;
  void *initrd;
  loom_kernel_module_t *modules;
  void *data;
} loom_kernel_loader_t;

void EXPORT (loom_kernel_loader_add) (loom_kernel_loader_t *kernel_loader);
int EXPORT (loom_kernel_loader_remove) (loom_bool_t free);

int EXPORT (loom_kernel_loader_boot) (void);

#endif