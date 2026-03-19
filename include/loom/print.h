#ifndef LOOM_PRINT_H
#define LOOM_PRINT_H 1

#include <stdarg.h>

#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/types.h"

typedef void (*loom_write_fn) (loom_write_buffer wbufs[], void *data);

usize export (loom_bvprintf) (loom_write_fn write_fn, void *data,
                              const char *fmt, va_list args);
usize export (loom_vsnprintf) (char *s, usize n, const char *fmt,
                               va_list args);
usize printf_func (3, 4) export (loom_snprintf) (char *s, usize n,
                                                 const char *fmt, ...);
usize export (loom_vprintf) (const char *fmt, va_list args);
usize printf_func (1, 2) export (loom_printf) (const char *fmt, ...);
usize printf_func (1, 2) loom_wprintf (const char *fmt, ...);

#endif