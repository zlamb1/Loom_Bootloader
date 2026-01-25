#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "compiler.h"
#include "error.h"
#include "types.h"

void EXPORT (loom_memcpy) (void *restrict dst, const void *restrict src,
                           loom_usize_t count);
void EXPORT (loom_memmove) (void *dst, const void *src, loom_usize_t count);
int EXPORT (loom_memcmp) (const void *lhs, const void *rhs,
                          loom_usize_t count);

loom_usize_t EXPORT (loom_strlen) (const char *s);
int EXPORT (loom_strcmp) (const char *s1, const char *s2);
void EXPORT (loom_strlower) (char *s);
loom_error_t EXPORT (loom_strtoi) (char *s, int *out);

char *EXPORT (loom_strdup) (const char *s);

#endif