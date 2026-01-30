#include "loom/kernel_loader.h"
#include "loom/mm.h"

static loom_kernel_loader_t *kernel_loader;

void
loom_kernel_loader_add (loom_kernel_loader_t *new_kernel_loader)
{
  loom_kernel_loader_remove (1);
  kernel_loader = new_kernel_loader;
}

int
loom_kernel_loader_remove (loom_bool_t free)
{
  if (!kernel_loader)
    return -1;

  if (free)
    {
      if (kernel_loader->kernel)
        {
          loom_free (kernel_loader->kernel);
          kernel_loader->kernel = NULL;
        }

      if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_INITRD
          && kernel_loader->initrd)
        {
          loom_free (kernel_loader->initrd);
          kernel_loader->initrd = NULL;
        }

      if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_MODULES)
        {
          loom_kernel_module_t *kernel_module = kernel_loader->modules;

          while (kernel_module)
            {
              loom_kernel_module_t *tmp = kernel_module->next;
              loom_free (kernel_module->data);
              loom_free (kernel_module);
              kernel_module = tmp;
            }

          kernel_loader->modules = NULL;
        }
    }

  kernel_loader = NULL;

  return 0;
}

int
loom_kernel_loader_boot (void)
{
  if (!kernel_loader)
    return -1;

  kernel_loader->boot (kernel_loader);

  return -1;
}