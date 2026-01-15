#include "loom/string.h"

void
loom_memcpy (char *restrict dst, const char *restrict src, loom_usize_t count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loom_panic ("loom_memcpy");

  for (loom_usize_t i = 0; i < count; ++i)
    *d++ = *s++;
}

void
loom_memmove (char *dst, const char *src, loom_usize_t count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loom_panic ("loom_memmove");

  if ((loom_address_t) s < (loom_address_t) d)
    while (count--)
      d[count] = s[count];
  else
    for (loom_usize_t i = 0; i < count; ++i)
      d[i] = s[i];
}

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
loom_strlower (char *s)
{
  while (s[0])
    {
      if (s[0] >= 'A' && s[0] <= 'Z')
        s[0] += 0x20;
      ++s;
    }
}

static int
chartoi (int base, char ch)
{
  if ((base == 2 && (ch == '0' || ch == '1'))
      || (base == 8 && (ch >= '0' && ch <= '7'))
      || ((base == 10 || base == 16) && (ch >= '0' && ch <= '9')))
    return ch - '0';
  else if (base == 16 && (ch >= 'a' && ch <= 'f'))
    return (ch - 'a') + 10;
  else if (base == 16 && (ch >= 'A' && ch <= 'F'))
    return (ch - 'A') + 10;
  else
    return -1;
}

loom_error_t
loom_strtoi (char *s, int *out)
{
  int base = 10, z = 0, n = 0, neg = 0;
  char ch;

  if (!s[0])
    return LOOM_ERR_BAD_ARG;

  while ((ch = s[0]))
    {
      int prev, cv;

      cv = chartoi (base, ch);

      if (cv < 0)
        {
          if (z == 0 && ch == '-')
            {
              neg = 1;
              goto next;
            }
          else if (z == 1)
            {
              if (ch == 'b' || ch == 'B')
                {
                  base = 2;
                  goto next;
                }
              if (ch == 'x' || ch == 'X')
                {
                  base = 16;
                  goto next;
                }
            }

          return LOOM_ERR_BAD_ARG;
        }

      if ((neg && n > INT_MIN / base) || (!neg && n > INT_MAX / base))
        return LOOM_ERR_OVERFLOW;

      prev = n;
      n *= base;

      if ((neg && n < INT_MIN + cv) || (!neg && n > INT_MAX - cv))
        return LOOM_ERR_OVERFLOW;

      n += neg ? -cv : cv;

      if (!n)
        ++z;
      else if (!prev)
        {
          if (base == 10 && z == 1)
            {
              base = 8;
              if (chartoi (8, ch) < 0)
                return LOOM_ERR_BAD_ARG;
            }
        }

    next:
      s++;
    }

  *out = n;
  return LOOM_ERR_NONE;
}