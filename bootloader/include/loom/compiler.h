#ifndef LOOM_COMPILER_H
#define LOOM_COMPILER_H 1

#define NORETURN     __attribute__ ((noreturn))
#define PACKED       __attribute__ ((packed))
#define PRINTF(X, Y) __attribute__ ((format (printf, X, Y)))
#define UNUSED       __attribute__ ((unused))

#endif