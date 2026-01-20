#include "loom/error.h"
#include "loom/print.h"

#define ERROR_BUF_SIZE 64

loom_error_t loom_errno;

static char error_buf[ERROR_BUF_SIZE];

void
loom_error (loom_error_t error, const char *fmt, ...)
{
  loom_errno = error;

  if (fmt)
    {
      va_list args;
      va_start (args, fmt);
      loom_vsnprintf (error_buf, ERROR_BUF_SIZE, fmt, args);
      va_end (args);
    }
  else
    error_buf[0] = 0;
}

const char *
loom_error_get (void)
{
  if (!error_buf[0])
    loom_snprintf (error_buf, ERROR_BUF_SIZE, "%s",
                   loom_strerror (loom_errno));

  return error_buf;
}

void
loom_error_clear (void)
{
  loom_error (LOOM_ERR_NONE, NULL);
}

const char *
loom_strerror (loom_error_t error)
{
  switch (error)
    {
    case LOOM_ERR_NONE:
      return "Success";
    case LOOM_ERR_BAD_ARG:
      return "Bad argument";
    case LOOM_ERR_ALLOC:
      return "Out of memory";
    case LOOM_ERR_OVERFLOW:
      return "Value overflowed";
    case LOOM_ERR_RANGE:
      return "Value out of range";
    case LOOM_ERR_IO:
      return "I/O error";
    case LOOM_ERR_BAD_BLOCK_SIZE:
      return "Bad block size";
    default:
      return "Unknown error";
    }
}