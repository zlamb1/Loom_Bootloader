#ifndef LOOM_PRINT_H
#define LOOM_PRINT_H 1

#include <stdarg.h>

#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/types.h"

typedef void (*loom_write_fn) (loom_write_buffer wbufs[], void *data);

typedef struct
{
  loom_write_fn write;
  void *data;
} loom_writer;

usize export (loomWriterFormatV) (loom_writer w, const char *fmt,
                                  va_list args);
usize export (loomWriterFormat) (loom_writer w, const char *fmt, ...);
usize export (loomBufferFormatV) (char *s, usize n, const char *fmt,
                                  va_list args);
usize printf_func (3, 4) export (loomBufferFormat) (char *s, usize n,
                                                    const char *fmt, ...);
usize export (loomLogV) (const char *fmt, va_list args);
usize printf_func (1, 2) export (loomLog) (const char *fmt, ...);
usize export (loomLogLnV) (const char *fmt, va_list args);
usize printf_func (1, 2) export (loomLogLn) (const char *fmt, ...);

#endif