#ifndef LOOM_TYPES_H
#define LOOM_TYPES_H

#include <limits.h>

#include <stddef.h>
#include <stdint.h>

typedef uint8_t loom_uint8_t;
typedef uint16_t loom_uint16_t;
typedef uint32_t loom_uint32_t;
typedef uint64_t loom_uint64_t;

typedef int loom_bool_t;

typedef uint32_t loom_usize_t;
typedef uintptr_t loom_uintptr_t;

#define LOOM_USIZE_MAX UINT32_MAX

#endif