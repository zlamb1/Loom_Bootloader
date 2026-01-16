#ifndef LOOM_ENDIAN_H
#define LOOM_ENDIAN_H 1

#include "types.h"

#if defined(LOOM_LITTLE_ENDIAN) && defined(LOOM_BIG_ENDIAN)
#error "Either little or big endian support must be enabled. Not both."
#endif

#define loom_le16toh loom_htole16
#define loom_le32toh loom_htole32
#define loom_le64toh loom_htole64
#define loom_be16toh loom_htobe16
#define loom_be32toh loom_htobe32
#define loom_be64toh loom_htobe64

#if defined(LOOM_LITTLE_ENDIAN)

static inline loom_uint16_t
loom_htole16 (loom_uint16_t n)
{
  return n;
}

static inline loom_uint32_t
loom_htole32 (loom_uint32_t n)
{
  return n;
}

static inline loom_uint64_t
loom_htole64 (loom_uint64_t n)
{
  return n;
}

static inline loom_uint16_t
loom_htobe16 (loom_uint16_t n)
{
  return __builtin_bswap16 (n);
}

static inline loom_uint32_t
loom_htobe32 (loom_uint32_t n)
{
  return __builtin_bswap32 (n);
}

static inline loom_uint64_t
loom_htobe64 (loom_uint64_t n)
{
  return __builtin_bswap64 (n);
}

#elif defined(LOOM_BIG_ENDIAN)

static inline loom_uint16_t
loom_htole16 (loom_uint16_t n)
{
  return __builtin_bswap16 (n);
}

static inline loom_uint32_t
loom_htole32 (loom_uint32_t n)
{
  return __builtin_bswap32 (n);
}

static inline loom_uint64_t
loom_htole64 (loom_uint64_t n)
{
  return __builtin_bswap64 (n);
}

static inline loom_uint16_t
loom_htobe16 (loom_uint16_t n)
{
  return n;
}

static inline loom_uint32_t
loom_htobe32 (loom_uint32_t n)
{
  return n;
}

static inline loom_uint64_t
loom_htobe64 (loom_uint64_t n)
{
  return n;
}

#else

#error Unknown Endianness

#endif

#endif