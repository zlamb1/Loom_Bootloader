#ifndef LOOM_STRING_H
#define LOOM_STRING_H 1

#include "compiler.h"
#include "types.h"

static UNUSED loom_usize
loom_strlen (const char *s)
{
  loom_usize len = 0;
  for (; s[len]; ++len)
    ;
  return len;
}

#endif