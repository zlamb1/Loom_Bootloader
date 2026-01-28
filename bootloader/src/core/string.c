#include "loom/string.h"
#include "loom/math.h"
#include "loom/mm.h"

void
loom_memcpy (void *restrict dst, const void *restrict src, loom_usize_t count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loom_panic ("memcpy");

  for (loom_usize_t i = 0; i < count; ++i)
    *d++ = *s++;
}

void
loom_memmove (void *dst, const void *src, loom_usize_t count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loom_panic ("memmove");

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
  if (!s)
    loom_panic ("strlen");

  loom_usize_t len = 0;
  for (; s[len]; ++len)
    ;
  return len;
}

int
loom_strcmp (const char *s1, const char *s2)
{
  unsigned char c1, c2;

  if (!s1 || !s2)
    loom_panic ("strcmp");

read:
  c1 = (unsigned char) (*s1++);
  c2 = (unsigned char) (*s2++);

  if (c1 != c2)
    return (int) c1 - (int) c2;

  if (c1 == '\0')
    return 0;

  goto read;
}

void
loom_strlower (char *s)
{
  if (!s)
    loom_panic ("strlower");

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
    loom_panic ("strtoi");

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

      prev = n;

      if (loom_mul (n, base, &n))
        return LOOM_ERR_OVERFLOW;

      if (loom_add (n, neg ? -cv : cv, &n))
        return LOOM_ERR_OVERFLOW;

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

  // Reject sign-only case (e.g '-')
  if (neg && !n && !z)
    return LOOM_ERR_BAD_ARG;

  *out = n;
  return LOOM_ERR_NONE;
}

int
loom_memcmp (const void *lhs, const void *rhs, loom_usize_t count)
{
  const unsigned char *l = lhs;
  const unsigned char *r = rhs;

  if (!lhs || !rhs)
    loom_panic ("memcmp");

  while (count--)
    {
      if (*l != *r)
        return (int) *l - (int) *r;
      ++l;
      ++r;
    }

  return 0;
}

char *
loom_strdup (const char *s)
{
  loom_usize_t len = loom_strlen (s);
  char *dups;

  if (!s)
    loom_panic ("strdup");

  if (len == LOOM_USIZE_MAX || !(dups = loom_malloc (len + 1)))
    return NULL;
  loom_memcpy (dups, s, len);
  dups[len] = '\0';
  return dups;
}