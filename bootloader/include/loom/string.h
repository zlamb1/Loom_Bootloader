#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "compiler.h"
#include "error.h"
#include "types.h"

void EXPORT (loom_memcpy) (char *restrict dst, const char *restrict src,
                           loom_usize_t count);
void EXPORT (loom_memmove) (char *dst, const char *src, loom_usize_t count);

loom_usize_t EXPORT (loom_strlen) (const char *s);
loom_bool_t EXPORT (loom_streq) (const char *s1, const char *s2);
void EXPORT (loom_strlower) (char *s);
loom_error_t EXPORT (loom_strtoi) (char *s, int *out);

#endif