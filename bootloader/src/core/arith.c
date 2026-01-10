#include <stdint.h>

uint64_t
__udivmoddi4 (uint64_t n, uint64_t d, uint64_t *rem)
{
  uint64_t r = 0, q = 0;

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