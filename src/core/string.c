#include "loom/string.h"
#include "loom/math.h"
#include "loom/mm.h"

void
loomMemCopy (void *restrict dst, const void *restrict src, usize count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loomPanic ("loomMemCopy");

  for (usize i = 0; i < count; ++i)
    *d++ = *s++;
}

void
loomMemMove (void *dst, const void *src, usize count)
{
  char *d = dst;
  const char *s = src;

  if (!d || !s)
    loomPanic ("loomMemMove");

  if ((address) s < (address) d)
    while (count--)
      d[count] = s[count];
  else
    for (usize i = 0; i < count; ++i)
      d[i] = s[i];
}

int
loomMemCmp (const void *lhs, const void *rhs, usize count)
{
  const unsigned char *l = lhs;
  const unsigned char *r = rhs;

  if (!lhs || !rhs)
    loomPanic ("loomMemCmp");

  while (count--)
    {
      if (*l != *r)
        return (int) *l - (int) *r;
      ++l;
      ++r;
    }

  return 0;
}

void
loomMemSet (void *dst, int v, usize n)
{
  unsigned char *d = dst;
  unsigned char vc = (unsigned char) v;

  if (!dst)
    loomPanic ("loomMemSet");

  while (n--)
    *d++ = vc;
}

usize
loomStrLength (const char *s)
{
  if (!s)
    loomPanic ("loomStrLength");

  usize len = 0;
  for (; s[len]; ++len)
    ;
  return len;
}

int
loomStrCmp (const char *s1, const char *s2)
{
  unsigned char c1, c2;

  if (!s1 || !s2)
    loomPanic ("loomStrCmp");

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
loomStrLower (char *s)
{
  if (!s)
    loomPanic ("loomStrLower");

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

loom_error
loomParseInt (const char *s, int *out)
{
  int base = 10, z = 0, n = 0, neg = 0;
  char ch;

  if (!s[0])
    loomPanic ("strtoi");

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

      if (loomMul (n, base, &n))
        return LOOM_ERR_OVERFLOW;

      if (loomAdd (n, neg ? -cv : cv, &n))
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

char *
loomStrDup (const char *s)
{
  usize len = loomStrLength (s);
  char *dups;

  if (!s)
    loomPanic ("strdup");

  if (len == USIZE_MAX || !(dups = loomAlloc (len + 1)))
    return NULL;
  loomMemCopy (dups, s, len);
  dups[len] = '\0';
  return dups;
}