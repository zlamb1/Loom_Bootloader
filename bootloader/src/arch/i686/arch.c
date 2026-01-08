#include "loom/arch.h"

void loom_vga_con_register (void);

void
loom_arch_init (void)
{
  loom_vga_con_register ();
}