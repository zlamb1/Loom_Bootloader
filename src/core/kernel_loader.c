#include "loom/kernel_loader.h"
#include "loom/mm.h"

loom_kernel_loader *kernel_loader = null;

void
loomKernelLoaderSet (loom_kernel_loader *new_kernel_loader)
{
  loomKernelLoaderRemove ();
  kernel_loader = new_kernel_loader;
}

int
loomKernelLoaderRemove (void)
{
  if (kernel_loader == null)
    return -1;

  auto kernel = &kernel_loader->kernel;
  auto initrd = &kernel_loader->initrd;
  auto cmdline = &kernel_loader->cmdline;

  if (kernel->data != null)
    {
      loomFree (kernel->data);
      kernel->size = 0;
      kernel->data = NULL;
    }

  if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_INITRD
      && initrd->data != null)
    {
      loomFree (initrd->data);
      initrd->size = 0;
      initrd->data = null;
    }

  if (cmdline->data != null)
    {
      loomFree (cmdline->data);
      cmdline->size = 0;
      cmdline->data = null;
    }

  if (kernel_loader->flags & LOOM_KERNEL_LOADER_FLAG_MODULES)
    {
      loom_kernel_module *module = kernel_loader->modules;

      while (module != null)
        {
          loom_kernel_module *tmp = module->next;
          loomFree (module->data);
          loomFree (module);
          module = tmp;
        }

      kernel_loader->modules = null;
    }

  kernel_loader = null;

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