#include "loom/types.h"

loom_uint64_t
__udivmoddi4 (loom_uint64_t n, loom_uint64_t d, loom_uint64_t *rem)
{
  loom_uint64_t r = 0, q = 0;

  for (int i = 63; i >= 0; i--)
    {
      r <<= 1;
      r |= (n >> i) & 1;
      if (r >= d)
        {
          r -= d;
          q |= 1LLU << i;
        }
    }

  *rem = r;
  return q;
}

loom_uint64_t
__udivdi3 (loom_uint32_t n, loom_uint64_t d)
{
  loom_uint64_t r = 0, q = 0;

  for (int i = 63; i >= 0; i--)
    {
      r <<= 1;
      r |= (n >> i) & 1;
      if (r >= d)
        {
          r -= d;
          q |= 1LLU << i;
        }
    }

  return q;
}

loom_uint64_t
__umoddi3 (loom_uint32_t n, loom_uint64_t d)
{
  loom_uint64_t r = 0;

  for (int i = 63; i >= 0; i--)
    {
      r <<= 1;
      r |= (n >> i) & 1;
      if (r >= d)
        r -= d;
    }

  return r;
}