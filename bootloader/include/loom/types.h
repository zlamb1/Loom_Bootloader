#ifndef LOOM_TYPES_H
#define LOOM_TYPES_H

#include <limits.h>

#include <stddef.h>
#include <stdint.h>

typedef uint8_t loom_u8;
typedef uint16_t loom_u16;
typedef uint32_t loom_u32;
typedef uint64_t loom_u64;

typedef uint32_t loom_usize;

#define LOOM_USIZE_MAX UINT32_MAX

#endif