#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/types.h"

void weak export (loom_memcpy) (void *restrict dst, const void *restrict src,
                                usize n);

void export (loom_memmove) (void *dst, const void *src, usize n);

int export (loom_memcmp) (const void *lhs, const void *rhs, usize n);

void weak export (loom_memset) (void *dst, int v, usize n);

usize export (loom_strlen) (const char *s);
int export (loom_strcmp) (const char *s1, const char *s2);
void export (loom_strlower) (char *s);
loom_error export (loom_strtoi) (char *s, int *out);

char *export (loom_strdup) (const char *s);

#endif