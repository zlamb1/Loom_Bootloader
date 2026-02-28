#ifndef LOOM_CRYPTO_H
#define LOOM_CRYPTO_H 1

#include "loom/print.h"
#include "loom/types.h"

#define loom_rotate_left(N, S)                                                \
  ({                                                                          \
    __auto_type __n = (N);                                                    \
    __auto_type __s = (S);                                                    \
    typeof (__n) __r;                                                         \
    if (!__s)                                                                 \
      __r = __n;                                                              \
    else                                                                      \
      {                                                                       \
        __s &= sizeof (__n) * 8 - 1;                                          \
        __r = (__n << __s) | (__n >> (sizeof (__n) * 8 - __s));               \
      }                                                                       \
    __r;                                                                      \
  })

typedef unsigned char loom_digest_t;

static inline void
loom_print_hash (loom_usize_t length, const loom_digest_t *digest)
{
  for (loom_usize_t i = 0; i < length; ++i)
    loom_printf ("%02hhx", digest[i]);
}

#endif