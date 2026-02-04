#if !defined(LOOM_I686_STDINT_H) && !defined(LOOM_NOSTDINT)
#define LOOM_I686_STDINT_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef uint32_t uintptr_t;
typedef int32_t intptr_t;

typedef uint64_t uintmax_t;
typedef int64_t intmax_t;

#define UINT8_MAX  255U
#define UINT16_MAX 65535U
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615ULL

#define INT8_MIN (-INT8_MAX - 1)
#define INT8_MAX 127

#define INT16_MIN (-INT16_MAX - 1)
#define INT16_MAX 32767

#define INT32_MIN (-INT32_MAX - 1)
#define INT32_MAX 2147483647

#define INT64_MIN (-INT64_MAX - 1LL)
#define INT64_MAX 9223372036854775807LL

#define UINTPTR_MAX UINT32_MAX

#define UINTMAX_MAX UINT64_MAX

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX

#endif