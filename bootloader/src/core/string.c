#include "loom/string.h"

loom_usize_t
loom_strlen (const char *s)
{
  loom_usize_t len = 0;
  for (; s[len]; ++len)
    ;
  return len;
}

loom_bool_t
loom_streq (const char *s1, const char *s2)
{
  if (!s1 || !s2)
    return 0;

  if (s1 == s2)
    return 1;

  while (1)
    {
      if (s1[0] != s2[0])
        return 0;
      else if (!s1[0])
        return 1;

      s1++;
      s2++;
    }
}

void
EXPORT (loom_strlower) (char *s)
{
  while (s[0])
    {
      if (s[0] >= 'A' && s[0] <= 'Z')
        s[0] += 0x20;
      ++s;
    }
}