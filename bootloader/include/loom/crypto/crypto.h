#ifndef LOOM_CRYPTO_H
#define LOOM_CRYPTO_H 1

#include "loom/print.h"
#include "loom/types.h"

static inline void
loom_printhash (loom_usize_t length, const char *digest)
{
  for (loom_usize_t i = 0; i < length; ++i)
    loom_printf ("%02hhx", digest[i]);
}

#endif