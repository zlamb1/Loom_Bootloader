#include "loom/assert.h"
#include "loom/error.h"
#include "loom/os.h"

void
_loom_assert (const char *cond, const char *file, uint line)
{
  loom_usize_t index = 0;

  for (loom_usize_t i = 0; file[i]; ++i)
    {
      if (file[i] == '/')
        index = i + 1;
#if defined(LOOM_HOST_OS) && LOOM_HOST_OS == LOOM_OS_WINDOWS
      if (file[i] == '\\')
        index = i + 1;
#endif
    }

  loom_panic ("%s:%u: Assertion '%s' failed.", file + index, line, cond);
}