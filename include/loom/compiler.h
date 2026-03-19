#ifndef LOOM_COMPILER_H
#define LOOM_COMPILER_H 1

#define compile_assert _Static_assert
#define assume_aligned __builtin_assume_aligned
#define alignof        _Alignof

#define _container_of(ptr, type, member)                                      \
  ({                                                                          \
    const typeof (((type *) 0)->member) *_tmpptr = (ptr);                     \
    (type *) assume_aligned ((char *) _tmpptr - offsetof (type, member),      \
                             alignof (type));                                 \
  })

#define container_of(ptr, type, member)                                       \
  _Generic ((ptr),                                                            \
      const typeof (*(ptr)) *: (const type *) _container_of (ptr, type,       \
                                                             member),         \
      default: _container_of (ptr, type, member))

#define alias(NAME)       __attribute__ ((alias (#NAME)))
#define align(N)          __attribute__ ((aligned (N)))
#define export(NAME)      NAME
#define export_var(NAME)  NAME
#define force_inline      __attribute__ ((always_inline))
#define noinline          __attribute__ ((noinline))
#define noreturn          __attribute__ ((noreturn))
#define packed            __attribute__ ((packed))
#define printf_func(X, Y) __attribute__ ((format (printf, X, Y)))
#define section(NAME)     __attribute ((section (#NAME)))
#define unused            __attribute__ ((unused))
#define used              __attribute__ ((used))
#define weak              __attribute__ ((weak))

#endif