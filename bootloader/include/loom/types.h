#ifndef LOOM_TYPES_H
#define LOOM_TYPES_H

#include <limits.h>

#include <stddef.h>
#include <stdint.h>

typedef uint8_t loom_uint8_t;
typedef uint16_t loom_uint16_t;
typedef uint32_t loom_uint32_t;
typedef uint64_t loom_uint64_t;

typedef int8_t loom_int8_t;
typedef int16_t loom_int16_t;
typedef int32_t loom_int32_t;
typedef int64_t loom_int64_t;

typedef int loom_bool_t;

typedef uint32_t loom_usize_t;
typedef uintptr_t loom_uintptr_t;
typedef loom_uintptr_t loom_address_t;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#define LOOM_UINT8_MAX  UINT8_MAX
#define LOOM_UINT16_MAX UINT16_MAX
#define LOOM_UINT32_MAX UINT32_MAX
#define LOOM_UINT64_MAX UINT64_MAX

#define LOOM_INT8_MIN  INT8_MIN
#define LOOM_INT16_MIN INT16_MIN
#define LOOM_INT32_MIN INT32_MIN
#define LOOM_INT64_MIN INT64_MIN

#define LOOM_INT8_MAX  INT8_MAX
#define LOOM_INT16_MAX INT16_MAX
#define LOOM_INT32_MAX INT32_MAX
#define LOOM_INT64_MAX INT64_MAX

#define LOOM_USIZE_MAX   UINT32_MAX
#define LOOM_ADDRESS_MAX UINTPTR_MAX

#endif