#ifndef LOOM_COMPILER_H
#define LOOM_COMPILER_H 1

#define loom_compile_assert _Static_assert
#define loom_assume_aligned __builtin_assume_aligned
#define loom_alignof        _Alignof

#define _loom_container_of(ptr, type, member)                                 \
  ({                                                                          \
    const typeof (((type *) 0)->member) *_tmpptr = (ptr);                     \
    (type *) __builtin_assume_aligned (                                       \
        (char *) _tmpptr - offsetof (type, member), loom_alignof (type));     \
  })

#define loom_container_of(ptr, type, member)                                  \
  _Generic ((ptr),                                                            \
      const typeof (*(ptr)) *: (const type *) _loom_container_of (ptr, type,  \
                                                                  member),    \
      default: _loom_container_of (ptr, type, member))

#define LOOM_ALIAS(NAME)      __attribute__ ((alias (#NAME)))
#define LOOM_ALIGNED(N)       __attribute__ ((aligned (N)))
#define LOOM_EXPORT(NAME)     NAME
#define LOOM_EXPORT_VAR(NAME) NAME
#define LOOM_NOINLINE         __attribute__ ((noinline))
#define LOOM_NORETURN         __attribute__ ((noreturn))
#define LOOM_PACKED           __attribute__ ((packed))
#define LOOM_PRINTF(X, Y)     __attribute__ ((format (printf, X, Y)))
#define LOOM_SECTION(NAME)    __attribute ((section (#NAME)))
#define LOOM_UNUSED           __attribute__ ((unused))
#define LOOM_USED             __attribute__ ((used))
#define LOOM_WEAK             __attribute__ ((weak))

#endif