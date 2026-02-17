#ifndef LOOM_ASSERT_H
#define LOOM_ASSERT_H 1

#include "loom/error.h"

#ifdef LOOM_DEBUG

#define loom_assert(COND)                                                     \
  do                                                                          \
    {                                                                         \
      if (!(COND))                                                            \
        _loom_assert (#COND, __FILE__, __LINE__);                             \
    }                                                                         \
  while (0)

#else

#define loom_assert(COND)

#endif

void _loom_assert (const char *cond, const char *file, uint line);

#endif