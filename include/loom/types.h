#ifndef LOOM_TYPES_H
#define LOOM_TYPES_H

#include <limits.h>
#include <stddef.h>

#ifdef LOOM_NOSTDINT
#include <stdint.h>
#else
#include "loom_stdint.h"
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ullong;

typedef uchar byte;

#define false 0
#define true  1

typedef u8 b8;
typedef u16 b16;
typedef u32 b32;
typedef uint bool;

typedef u32 usize;
typedef uintptr_t uintptr;
typedef uintptr address;

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define I8_MIN  INT8_MIN
#define I16_MIN INT16_MIN
#define I32_MIN INT32_MIN
#define I64_MIN INT64_MIN

#define I8_MAX  INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX

#define USIZE_MAX   U32_MAX
#define ADDRESS_MAX UINTPTR_MAX

#endif