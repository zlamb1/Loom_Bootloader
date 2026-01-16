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
typedef loom_uintptr_t loom_address_t;

#define LOOM_UINT8_MAX  UINT8_MAX
#define LOOM_UINT16_MAX UINT16_MAX
#define LOOM_UINT32_MAX UINT32_MAX
#define LOOM_UINT64_MAX UINT64_MAX

#define LOOM_USIZE_MAX UINT32_MAX

#endif