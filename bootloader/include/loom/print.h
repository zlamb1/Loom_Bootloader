#ifndef LOOM_PRINT_H
#define LOOM_PRINT_H 1

#include <stdarg.h>

#include "compiler.h"
#include "types.h"

loom_usize loom_vprintf(const char *fmt, va_list args);
loom_usize PRINTF(1,2) loom_printf(const char *fmt, ...);
loom_usize PRINTF(1,2) loom_wprintf(const char *fmt, ...);

#endif