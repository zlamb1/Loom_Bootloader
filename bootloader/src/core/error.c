#include "loom/error.h"

#define LOOM_ERR_NONE           0
#define LOOM_ERR_BAD_ARG        1
#define LOOM_ERR_ALLOC          2
#define LOOM_ERR_OVERFLOW       3
#define LOOM_ERR_RANGE          4
#define LOOM_ERR_IO             5
#define LOOM_ERR_BAD_BLOCK_SIZE 6

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