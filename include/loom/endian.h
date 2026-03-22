#ifndef LOOM_ENDIAN_H
#define LOOM_ENDIAN_H 1

#include "loom/compiler.h"
#include "loom/types.h"

#ifdef LOOM_UTIL
#ifndef __BYTE_ORDER__
#error "Define byte order for host via __BYTE_ORDER__"
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LOOM_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LOOM_BIG_ENDIAN
#else
#error "Unsupported host byte order."
#endif
#endif

#if !defined(LOOM_LITTLE_ENDIAN) && !defined(LOOM_BIG_ENDIAN)
#error "Endianness must be defined."
#endif

#if defined(LOOM_LITTLE_ENDIAN) && defined(LOOM_BIG_ENDIAN)
#error "Either little or big endian support must be enabled. Not both."
#endif

#define _ENDIAN_STRUCTS_EXT(X1, X2)                                           \
  X1 (u16le, u16, __htole16)                                                  \
  X1 (u32le, u32, __htole32)                                                  \
  X1 (u64le, u64, __htole64)                                                  \
  X1 (u16be, u16, __htobe16)                                                  \
  X1 (u32be, u32, __htobe32)                                                  \
  X2 (u64be, u64, __htobe64)

#define _ENDIAN_STRUCTS _ENDIAN_STRUCTS_EXT (X, X)

#define X(name, type, conv)                                                   \
  typedef struct                                                              \
  {                                                                           \
    type v;                                                                   \
  } name;                                                                     \
  compile_assert (sizeof (name) == sizeof (type),                             \
                  "endian struct size mismatch for " #name);                  \
  compile_assert (alignof (name) == alignof (type),                           \
                  "endian struct alignment mismatch for " #name);

_ENDIAN_STRUCTS

#undef X

#ifdef LOOM_LITTLE_ENDIAN

#ifndef htole16
static inline u16 force_inline
htole16 (u16 u)
{
  return u;
}
#endif

#ifndef htole32
static inline u32 force_inline
htole32 (u32 u)
{
  return u;
}
#endif

#ifndef htole64
static inline u64 force_inline
htole64 (u64 u)
{
  return u;
}
#endif

#ifndef htobe16
static inline u16 force_inline
htobe16 (u16 u)
{
  return __builtin_bswap16 (u);
}
#endif

#ifndef htobe32
static inline u32 force_inline
htobe32 (u32 u)
{
  return __builtin_bswap32 (u);
}
#endif

#ifndef htobe64
static inline u64 force_inline
htobe64 (u64 u)
{
  return __builtin_bswap64 (u);
}
#endif

#endif

#ifdef LOOM_BIG_ENDIAN

#ifndef htole16
static inline u16 force_inline
htole16 (u16 u)
{
  return __builtin_bswap16 (u);
}
#endif

#ifndef htole32
static inline u32 force_inline
htole32 (u32 u)
{
  return __builtin_bswap32 (u);
}
#endif

#ifndef htole64
static inline u64 force_inline
htole64 (u64 u)
{
  return __builtin_bswap64 (u);
}
#endif

#ifndef htobe16
static inline u16 force_inline
htobe16 (u16 u)
{
  return u;
}
#endif

#ifndef htobe32
static inline u32 force_inline
htobe32 (u32 u)
{
  return u;
}
#endif

#ifndef htobe64
static inline u64 force_inline
htobe64 (u64 u)
{
  return u;
}
#endif

#endif

#ifndef le16toh
static inline u16 force_inline
le16toh (u16 u)
{
  return htole16 (u);
}
#endif

#ifndef le32toh
static inline u32 force_inline
le32toh (u32 u)
{
  return htole32 (u);
}
#endif

#ifndef le64toh
static inline u64 force_inline
le64toh (u64 u)
{
  return htole64 (u);
}
#endif

#ifndef be16toh
static inline u16 force_inline
be16toh (u16 u)
{
  return htobe16 (u);
}
#endif

#ifndef be32toh
static inline u32 force_inline
be32toh (u32 u)
{
  return htobe32 (u);
}
#endif

#ifndef be64toh
static inline u64 force_inline
be64toh (u64 u)
{
  return htobe64 (u);
}
#endif

/*
 * Note: Unfortunately we need to create another layer of indirection to handle
 * the fact that function-like macros cannot be used in _Generic. This is only
 * necessary for the host-side where le/bexxtoh and friends are potentially
 * implemented with function-like macros instead of static inline functions.
 */

#ifdef htole16
static inline u16 force_inline
__htole16 (u16 u)
{
  return htole16 (u);
}
#else
#define __htole16 htole16
#endif

#ifdef htole32
static inline u32 force_inline
__htole32 (u32 u)
{
  return htole32 (u);
}
#else
#define __htole32 htole32
#endif

#ifdef htole64
static inline u64 force_inline
__htole64 (u64 u)
{
  return htole64 (u);
}
#else
#define __htole64 htole64
#endif

#ifdef htobe16
static inline u16 force_inline
__htobe16 (u16 u)
{
  return htobe16 (u);
}
#else
#define __htobe16 htobe16
#endif

#ifdef htobe32
static inline u32 force_inline
__htobe32 (u32 u)
{
  return htobe32 (u);
}
#else
#define __htobe32 htobe32
#endif

#ifdef htobe64
static inline u64 force_inline
__htobe64 (u64 u)
{
  return htobe64 (u);
}
#else
#define __htobe64 htobe64
#endif

#define X(name, type, conv)                                                   \
  static inline type force_inline name##Load (name s) { return conv (s.v); }  \
  static inline type force_inline name##LoadP (const name *s)                 \
  {                                                                           \
    return conv (s->v);                                                       \
  }                                                                           \
  static inline type force_inline name##LoadPV (const volatile name *s)       \
  {                                                                           \
    return conv (s->v);                                                       \
  }                                                                           \
  static inline void force_inline name##Store (name *s, type v)               \
  {                                                                           \
    s->v = conv (v);                                                          \
  }                                                                           \
  static inline void force_inline name##StoreV (volatile name *s, type v)     \
  {                                                                           \
    s->v = conv (v);                                                          \
  }

_ENDIAN_STRUCTS

#undef X

// clang-format off

#define _ENDIAN_LOAD_GEN1(name, type, conv) \
  name: name##Load, \
  name *: name##LoadP, \
  const name *: name##LoadP, \
  volatile name *: name##LoadPV, \
  const volatile name *: name##LoadPV

#define _ENDIAN_LOAD_GEN2(name, type, conv) name: name##Load, \
  name *: name##LoadP, \
  const name *: name##LoadP, \
  volatile name *: name##LoadPV, \
  const volatile name *: name##LoadPV,

#define _ENDIAN_STORE1(name, type, conv) name: conv

#define _ENDIAN_STORE2(name, type, conv) name: conv,

// clang-format on

#define endianLoad(x)                                                         \
  _Generic ((x),                                                              \
      _ENDIAN_STRUCTS_EXT (_ENDIAN_LOAD_GEN2, _ENDIAN_LOAD_GEN1)) (x)

#define endianStore(x, _v)                                                    \
  x.v = _Generic ((x),                                                        \
      _ENDIAN_STRUCTS_EXT (_ENDIAN_STORE2, _ENDIAN_STORE1)) (_v)

#endif