#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOM_NOSTDINT
#include "loom/error.h"
#include "loom/mm.h"
#include "loom/string.h"

loom_error loom_errno;

#define ERROR_BUF_SIZE 64
static char error_buf[ERROR_BUF_SIZE];

loom_error
loomErrorFmt (loom_error error, const char *fmt, ...)
{
  loom_errno = error;

  if (fmt)
    {
      va_list args;
      va_start (args, fmt);
      vsnprintf (error_buf, ERROR_BUF_SIZE, fmt, args);
      va_end (args);
    }
  else
    error_buf[0] = '\0';

  return error;
}

const char *
loomErrorGet (void)
{
  if (error_buf[0] == '\0')
    snprintf (error_buf, ERROR_BUF_SIZE, "%s", loomStringError (loom_errno));

  return error_buf;
}

void
loomErrorClear (void)
{
  loomErrorFmt (LOOM_ERR_NONE, NULL);
}

void
loomMemMove (void *dst, const void *src, usize n)
{
  memcpy (dst, src, n);
}

void *
loomAlloc (usize size)
{
  return malloc (size);
}

void
loomFree (void *p)
{
  free (p);
}