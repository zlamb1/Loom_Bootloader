#include "loom/main.h"
#include "loom/commands/core.h"
#include "loom/module.h"
#include "loom/platform.h"
#include "loom/shell.h"
#include "loom/symbol.h"

void
loomMain (void)
{
  loomPlatformInit ();
  loomMmapInit ();
  loomRegisterExportSymbols ();
  loomCoreCommandsInit ();
  loomCoreModulesLoad ();
  loomShellExec ();
  for (;;)
    ;
}