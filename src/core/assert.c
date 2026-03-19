#include "loom/assert.h"
#include "loom/error.h"
#include "loom/os.h"

void
_loomAssert (const char *cond, const char *file, uint line)
{
  usize index = 0;

  for (usize i = 0; file[i]; ++i)
    {
      if (file[i] == '/')
        index = i + 1;
#if defined(LOOM_HOST_OS) && LOOM_HOST_OS == LOOM_OS_WINDOWS
      if (file[i] == '\\')
        index = i + 1;
#endif
    }

  loomPanic ("%s:%u: Assertion '%s' failed.", file + index, line, cond);
}