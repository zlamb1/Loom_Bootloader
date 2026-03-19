#include "loom/error.h"
#include "loom/print.h"

#define ERROR_BUF_SIZE 64

loom_error loom_errno;

static char error_buf[ERROR_BUF_SIZE];

loom_error
loomErrorFmt (loom_error error, const char *fmt, ...)
{
  loom_errno = error;

  if (fmt)
    {
      va_list args;
      va_start (args, fmt);
      loomBufferFormatV (error_buf, ERROR_BUF_SIZE, fmt, args);
      va_end (args);
    }
  else
    error_buf[0] = 0;

  return error;
}

const char *
loomErrorGet (void)
{
  if (!error_buf[0])
    loomBufferFormat (error_buf, ERROR_BUF_SIZE, "%s",
                      loomStringError (loom_errno));

  return error_buf;
}

void
loomErrorClear (void)
{
  loomErrorFmt (LOOM_ERR_NONE, NULL);
}

const char *
loomStringError (loom_error error)
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
    case LOOM_ERR_BAD_PART_SCHEME:
      return "Bad partition scheme";
    case LOOM_ERR_HOOK:
      return "Bad hook";
    default:
      return "Unknown error";
    }
}