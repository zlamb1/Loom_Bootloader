#include "loom/crypto/sha1.h"
#include "loom/crypto/crypto.h"
#include "loom/endian.h"
#include "loom/string.h"

typedef struct
{
  u32 w[80];
  u32 h[5];
} sha1_context;

static void
processChunk (sha1_context *ctx)
{
  u32 *w = ctx->w, a, b, c, d, e, f, k, tmp;

  for (unsigned int i = 0; i < 16; ++i)
    w[i] = loom_be32toh (w[i]);

  for (unsigned int i = 16; i < 80; ++i)
    {
      w[i] = (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
      w[i] = loomRotateLeft (w[i], 1u);
    }

  a = ctx->h[0];
  b = ctx->h[1];
  c = ctx->h[2];
  d = ctx->h[3];
  e = ctx->h[4];

  for (unsigned int i = 0; i < 80; ++i)
    {
      if (i <= 19)
        {
          f = (b & c) | (~b & d);
          k = 0x5A827999;
        }
      else if (i <= 39)
        {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
        }
      else if (i <= 59)
        {
          f = (b & c) | (b & d) | (c & d);
          k = 0x8F1BBCDC;
        }
      else
        {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
        }

      tmp = loomRotateLeft (a, 5u) + f + e + k + w[i];
      e = d;
      d = c;
      c = loomRotateLeft (b, 30u);
      b = a;
      a = tmp;
    }

  ctx->h[0] += a;
  ctx->h[1] += b;
  ctx->h[2] += c;
  ctx->h[3] += d;
  ctx->h[4] += e;
}

void
loomSHA1Hash (usize length, const char *buf, loom_digest digest[20])
{
  // See: https://en.wikipedia.org/wiki/SHA-1#SHA-1_pseudocode

  usize original_length = length;

  sha1_context ctx;
  unsigned char *w_bytes;

  ctx.h[0] = 0x67452301;
  ctx.h[1] = 0xEFCDAB89;
  ctx.h[2] = 0x98BADCFE;
  ctx.h[3] = 0x10325476;
  ctx.h[4] = 0xC3D2E1F0;

  while (length >= 64)
    {
      loomMemCopy ((void *) ctx.w, buf, 64);
      processChunk (&ctx);
      length -= 64;
      buf += 64;
    }

  loomMemCopy ((void *) ctx.w, buf, length);
  w_bytes = (unsigned char *) ctx.w;
  w_bytes[length] = 0x80;

  for (unsigned int i = length + 1; i < 64; ++i)
    w_bytes[i] = 0;

  if (length >= 56)
    {
      processChunk (&ctx);
      for (int i = 0; i < 14; ++i)
        ctx.w[i] = 0;
    }

  u64 message_length = original_length * 8;
  ctx.w[14] = loom_htobe32 ((u32) (message_length >> 32));
  ctx.w[15] = loom_htobe32 ((u32) message_length);
  processChunk (&ctx);

  for (unsigned int i = 0; i < 5; ++i)
    {
      u32 h = loom_htobe32 (ctx.h[i]);
      loomMemCopy (digest + i * 4, &h, 4);
    }
}