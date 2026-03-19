#ifndef LOOM_ASSERT_H
#define LOOM_ASSERT_H 1

#include "loom/error.h"

#ifdef LOOM_DEBUG

#define loomAssert(COND)                                                      \
  do                                                                          \
    {                                                                         \
      if (!(COND))                                                            \
        _loomAssert (#COND, __FILE__, __LINE__);                              \
    }                                                                         \
  while (0)

#else

#define loomAssert(COND)

#endif

void export (_loomAssert) (const char *cond, const char *file, uint line);

#endif