#include "loom/types.h"

u64
__udivmoddi4 (u64 n, u64 d, u64 *rem)
{
  u64 r = 0, q = 0;

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

u64
__udivdi3 (u32 n, u64 d)
{
  u64 r = 0, q = 0;

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

u64
__umoddi3 (u32 n, u64 d)
{
  u64 r = 0;

  for (int i = 63; i >= 0; i--)
    {
      r <<= 1;
      r |= (n >> i) & 1;
      if (r >= d)
        r -= d;
    }

  return r;
}