#ifndef LOOM_ENDIAN_H
#define LOOM_ENDIAN_H 1

#include "loom/types.h"

#if defined(LOOM_LITTLE_ENDIAN) && defined(LOOM_BIG_ENDIAN)
#error "Either little or big endian support must be enabled. Not both."
#endif

static inline loom_uint16_t loom_htole16 (loom_uint16_t n);
static inline loom_uint32_t loom_htole32 (loom_uint32_t n);
static inline loom_uint64_t loom_htole64 (loom_uint64_t n);
static inline loom_uint16_t loom_htobe16 (loom_uint16_t n);
static inline loom_uint32_t loom_htobe32 (loom_uint32_t n);
static inline loom_uint64_t loom_htobe64 (loom_uint64_t n);

#define loom_le16toh loom_htole16
#define loom_le32toh loom_htole32
#define loom_le64toh loom_htole64
#define loom_be16toh loom_htobe16
#define loom_be32toh loom_htobe32
#define loom_be64toh loom_htobe64

static inline loom_int16_t
loom_le16toh_s (loom_int16_t v)
{
  return (loom_int16_t) loom_le16toh ((loom_uint16_t) v);
}

static inline loom_int32_t
loom_le32toh_s (loom_int32_t v)
{
  return (loom_int32_t) loom_le32toh ((loom_uint32_t) v);
}

static inline loom_int64_t
loom_le64toh_s (loom_int64_t v)
{
  return (loom_int64_t) loom_le64toh ((loom_uint64_t) v);
}

static inline loom_int16_t
loom_be16toh_s (loom_int16_t v)
{
  return (loom_int16_t) loom_be16toh ((loom_uint16_t) v);
}

static inline loom_int32_t
loom_be32toh_s (loom_int32_t v)
{
  return (loom_int32_t) loom_be32toh ((loom_uint32_t) v);
}

static inline loom_int64_t
loom_be64toh_s (loom_int64_t v)
{
  return (loom_int64_t) loom_be64toh ((loom_uint64_t) v);
}

#define ENDIAN_TYPE(name, type)                                               \
  typedef struct                                                              \
  {                                                                           \
    type v;                                                                   \
  } name;                                                                     \
  loom_compile_assert (sizeof (name) == sizeof (type)                         \
                           && loom_alignof (name) == loom_alignof (type),     \
                       "Invalid endian type.")

ENDIAN_TYPE (u16le, loom_uint16_t);
ENDIAN_TYPE (u32le, loom_uint32_t);
ENDIAN_TYPE (u64le, loom_uint64_t);
ENDIAN_TYPE (i16le, loom_int16_t);
ENDIAN_TYPE (i32le, loom_int32_t);
ENDIAN_TYPE (i64le, loom_int64_t);

ENDIAN_TYPE (u16be, loom_uint16_t);
ENDIAN_TYPE (u32be, loom_uint32_t);
ENDIAN_TYPE (u64be, loom_uint64_t);
ENDIAN_TYPE (i16be, loom_int16_t);
ENDIAN_TYPE (i32be, loom_int32_t);
ENDIAN_TYPE (i64be, loom_int64_t);

#define loom_letoh(x)                                                         \
  _Generic ((x),                                                              \
      u16le: loom_letoh16,                                                    \
      u32le: loom_letoh32,                                                    \
      u64le: loom_letoh64,                                                    \
      i16le: loom_letoh16_s,                                                  \
      i32le: loom_letoh32_s,                                                  \
      i64le: loom_letoh64_s) ((x).v)

#define loom_betoh(x)                                                         \
  _Generic ((x),                                                              \
      u16be: loom_betoh16,                                                    \
      u32be: loom_betoh32,                                                    \
      u64be: loom_betoh64,                                                    \
      i16be: loom_betoh16_s,                                                  \
      i32be: loom_betoh32_s,                                                  \
      i64be: loom_betoh64_s) ((x).v)

#undef ENDIAN_TYPE

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