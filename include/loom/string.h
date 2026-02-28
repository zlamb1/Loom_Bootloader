#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/types.h"

void LOOM_WEAK LOOM_EXPORT (loom_memcpy) (void *restrict dst,
                                          const void *restrict src,
                                          loom_usize_t n);

void LOOM_EXPORT (loom_memmove) (void *dst, const void *src, loom_usize_t n);

int LOOM_EXPORT (loom_memcmp) (const void *lhs, const void *rhs,
                               loom_usize_t n);

void LOOM_WEAK LOOM_EXPORT (loom_memset) (void *dst, int v, loom_usize_t n);

loom_usize_t LOOM_EXPORT (loom_strlen) (const char *s);
int LOOM_EXPORT (loom_strcmp) (const char *s1, const char *s2);
void LOOM_EXPORT (loom_strlower) (char *s);
loom_error_t LOOM_EXPORT (loom_strtoi) (char *s, int *out);

char *LOOM_EXPORT (loom_strdup) (const char *s);

#endif