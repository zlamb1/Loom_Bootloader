#include "loom/assert.h"
#include "loom/platform.h"
#include "loom/platform/x86/bios/bios.h"

static loom_bios_hook *hooks = null;

extern void loomBiosIntImpl (u8 intno, loom_bios_args *args);

void
loomBiosHookRegister (loom_bios_hook *hook)
{
  loomAssert (hook != null);
  loomAssert (hook->fn != null);
  hook->next = hooks;
  hooks = hook;
}

void
loomBiosHookUnregister (loom_bios_hook *hook)
{
  loom_bios_hook *current = hooks;

  loomAssert (hook != null);

  if (hooks == hook)
    {
      hooks = hook->next;
      hook->next = null;
      return;
    }

  while (current != null)
    {
      if (current->next == hook)
        {
          current->next = hook->next;
          hook->next = null;
          return;
        }
      current = current->next;
    }
}

void
loomBiosInt (u8 intno, loom_bios_args *args)
{
  loom_bios_hook *hook = hooks;
  int flags = loomIrqSave ();

  while (hook != null)
    {
      hook->fn (LOOM_BIOS_HOOK_TYPE_ENTER, hook->ctx);
      hook = hook->next;
    }

  loomBiosIntImpl (intno, args);

  hook = hooks;

  while (hook != null)
    {
      hook->fn (LOOM_BIOS_HOOK_TYPE_LEAVE, hook->ctx);
      hook = hook->next;
    }

  loomIrqRestore (flags);
}