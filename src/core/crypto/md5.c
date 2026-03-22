#include "loom/crypto/md5.h"
#include "loom/crypto/crypto.h"
#include "loom/endian.h"
#include "loom/string.h"

typedef struct
{
  u32 M[16];
  u32 h[4];
} md5_context;

static void
processChunk (md5_context *ctx)
{
  u32 s[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
  };

  u32 K[64] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf,
                0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af,
                0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e,
                0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6,
                0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
                0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122,
                0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039,
                0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97,
                0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d,
                0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

  u32 *M = ctx->M, a = ctx->h[0], b = ctx->h[1], c = ctx->h[2], d = ctx->h[3];

  for (unsigned int i = 0; i < 16; ++i)
    M[i] = le32toh (M[i]);

  for (unsigned int i = 0; i < 64; ++i)
    {
      u32 f, g;
      if (i <= 15)
        {
          f = (b & c) | (~b & d);
          g = i;
        }
      else if (i <= 31)
        {
          f = (b & d) | (c & ~d);
          g = (5 * i + 1) & 15;
        }
      else if (i <= 47)
        {
          f = b ^ c ^ d;
          g = (3 * i + 5) & 15;
        }
      else
        {
          f = c ^ (b | ~d);
          g = (7 * i) & 15;
        }

      f = f + a + K[i] + ctx->M[g];
      a = d;
      d = c;
      c = b;
      b += loomRotateLeft (f, s[i]);
    }

  ctx->h[0] += a;
  ctx->h[1] += b;
  ctx->h[2] += c;
  ctx->h[3] += d;
}

void
loomMD5Hash (usize length, const char *buf, loom_digest digest[16])
{
  // See: https://en.wikipedia.org/wiki/MD5#Algorithm

  usize original_length = length;

  md5_context ctx;

  unsigned char *M_bytes;

  ctx.h[0] = 0x67452301;
  ctx.h[1] = 0xefcdab89;
  ctx.h[2] = 0x98badcfe;
  ctx.h[3] = 0x10325476;

  while (length >= 64)
    {
      loomMemCopy ((void *) ctx.M, buf, 64);
      processChunk (&ctx);
      length -= 64;
      buf += 64;
    }

  loomMemCopy ((void *) ctx.M, buf, length);

  M_bytes = (unsigned char *) ctx.M;
  M_bytes[length] = 0x80;

  for (unsigned int i = length + 1; i < 64; ++i)
    M_bytes[i] = 0;

  if (length >= 56)
    {
      processChunk (&ctx);
      for (int i = 0; i < 14; ++i)
        ctx.M[i] = 0;
    }

  u64 message_length = original_length * 8;
  ctx.M[14] = htole32 ((u32) message_length);
  ctx.M[15] = htole32 ((u32) (message_length >> 32));
  processChunk (&ctx);

  for (unsigned int i = 0; i < 4; ++i)
    {
      u32 h = htole32 (ctx.h[i]);
      loomMemCopy (digest + i * 4, &h, 4);
    }
}