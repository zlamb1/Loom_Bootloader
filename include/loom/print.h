#ifndef LOOM_PRINT_H
#define LOOM_PRINT_H 1

#include <stdarg.h>

#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/types.h"

typedef void (*loom_write_fn) (loom_write_buffer wbufs[], void *data);

usize LOOM_EXPORT (loom_bvprintf) (loom_write_fn write_fn, void *data,
                                   const char *fmt, va_list args);
usize LOOM_EXPORT (loom_vsnprintf) (char *s, usize n, const char *fmt,
                                    va_list args);
usize LOOM_PRINTF (3, 4)
    LOOM_EXPORT (loom_snprintf) (char *s, usize n, const char *fmt, ...);
usize LOOM_EXPORT (loom_vprintf) (const char *fmt, va_list args);
usize LOOM_PRINTF (1, 2) LOOM_EXPORT (loom_printf) (const char *fmt, ...);
usize LOOM_PRINTF (1, 2) loom_wprintf (const char *fmt, ...);

#endif