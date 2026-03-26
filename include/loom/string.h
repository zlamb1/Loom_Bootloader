#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/types.h"

void weak export (loomMemCopy) (void *restrict dst, const void *restrict src,
                                usize n);

void export (loomMemMove) (void *dst, const void *src, usize n);

int export (loomMemCmp) (const void *lhs, const void *rhs, usize n);

void weak export (loomMemSet) (void *dst, int v, usize n);

usize export (loomStrLength) (const char *s);
int export (loomStrCmp) (const char *s1, const char *s2);
void export (loomStrLower) (char *s);
loom_error export (loomParseInt) (const char *s, int *out);

char *export (loomStrDup) (const char *s);

#endif