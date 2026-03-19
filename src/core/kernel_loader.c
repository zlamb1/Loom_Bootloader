#include "loom/kernel_loader.h"
#include "loom/mm.h"

static loom_kernel_loader *kernel_loader;

void
loomKernelLoaderSet (loom_kernel_loader *new_kernel_loader)
{
  loomKernelLoaderRemove (1);
  kernel_loader = new_kernel_loader;
}

int
loomKernelLoaderRemove (bool free)
{
  if (!kernel_loader)
    return -1;

  if (free)
    {
      if (kernel_loader->kernel)
        {
          loomFree (kernel_loader->kernel);
          kernel_loader->kernel = NULL;
        }

      if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_INITRD
          && kernel_loader->initrd)
        {
          loomFree (kernel_loader->initrd);
          kernel_loader->initrd = NULL;
        }

      if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_MODULES)
        {
          loom_kernel_module *kernel_module = kernel_loader->modules;

          while (kernel_module)
            {
              loom_kernel_module *tmp = kernel_module->next;
              loomFree (kernel_module->data);
              loomFree (kernel_module);
              kernel_module = tmp;
            }

          kernel_loader->modules = NULL;
        }
    }

  kernel_loader = NULL;

  return 0;
}

int
loomKernelLoaderBoot (void)
{
  if (!kernel_loader)
    return -1;

  kernel_loader->boot (kernel_loader);

  return -1;
}