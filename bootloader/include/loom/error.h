#ifndef LOOM_ERROR_H
#define LOOM_ERROR_H 1

#include "compiler.h"
#include "types.h"

#define LOOM_ERR_NONE    0
#define LOOM_ERR_BAD_ARG 1
#define LOOM_ERR_ALLOC   2

typedef loom_uint8_t loom_error_t;

void NORETURN EXPORT (loom_panic) (const char *msg);

#endif