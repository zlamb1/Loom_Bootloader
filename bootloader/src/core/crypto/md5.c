#include "loom/crypto/md5.h"
#include "loom/crypto/crypto.h"
#include "loom/endian.h"
#include "loom/string.h"

void
loom_md5_hash (loom_usize_t length, const char *buf, char digest[16])
{
  // See: https://en.wikipedia.org/wiki/MD5#Algorithm

  loom_usize_t remaining = length;
  loom_bool_t empty_chunk = 0;

  loom_uint32_t a0, b0, c0, d0, i, s[64]
      = { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
          5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
          4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
          6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, },
      K[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
      };

  a0 = 0x67452301;
  b0 = 0xefcdab89;
  c0 = 0x98badcfe;
  d0 = 0x10325476;

  do
    {
      loom_uint32_t M[16] = { 0 }, A = a0, B = b0, C = c0, D = d0,
                    read = remaining;

      if (empty_chunk)
        {
          loom_uint64_t bits;

          if (remaining != 64)
            loom_panic ("loom_md5_hash");

          bits = loom_htole64 (length * 8);
          M[14] = (loom_uint32_t) bits;
          M[15] = (loom_uint32_t) (bits >> 32);
          goto do_hash;
        }

      if (read > 64)
        read = 64;

      loom_memcpy ((void *) M, buf, read);

      if (read < 64)
        {
          unsigned char *M_bytes = (unsigned char *) M;
          M_bytes[read] = 0x80;

          if (read < 56)
            {
              loom_uint64_t bits = loom_htole64 (length * 8);
              M[14] = (loom_uint32_t) bits;
              M[15] = (loom_uint32_t) (bits >> 32);
            }
          else
            {
              empty_chunk = 1;
              remaining += 64;
            }
        }

    do_hash:
      for (i = 0; i < 16; ++i)
        M[i] = loom_htole32 (M[i]);

      for (i = 0; i < 64; ++i)
        {
          loom_uint32_t F, g;
          if (i <= 15)
            {
              F = (B & C) | (~B & D);
              g = i;
            }
          else if (i <= 31)
            {
              F = (B & D) | (C & ~D);
              g = (5 * i + 1) & 15;
            }
          else if (i <= 47)
            {
              F = B ^ C ^ D;
              g = (3 * i + 5) & 15;
            }
          else
            {
              F = C ^ (B | ~D);
              g = (7 * i) & 15;
            }

          F = F + A + K[i] + M[g];
          A = D;
          D = C;
          C = B;
          B += loom_rotate_left (F, s[i]);
        }

      a0 += A;
      b0 += B;
      c0 += C;
      d0 += D;

      if (!empty_chunk)
        buf += read;

      remaining -= read;
    }
  while (remaining);

  for (int j = 0; j < 4; ++j)
    {
      digest[j] = (char) (a0 >> (j * 8));
      digest[j + 4] = (char) (b0 >> (j * 8));
      digest[j + 8] = (char) (c0 >> (j * 8));
      digest[j + 12] = (char) (d0 >> (j * 8));
    }
}