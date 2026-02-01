#ifndef LOOM_COMPILER_H
#define LOOM_COMPILER_H 1

#define compile_assert      _Static_assert
#define loom_assume_aligned __builtin_assume_aligned
#define loom_alignof        _Alignof

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

#endif