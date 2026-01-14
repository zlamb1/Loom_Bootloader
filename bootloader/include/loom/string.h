#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "compiler.h"
#include "types.h"

loom_usize_t EXPORT (loom_strlen) (const char *s);
loom_bool_t EXPORT (loom_streq) (const char *s1, const char *s2);
void EXPORT (loom_strlower) (char *s);

#endif