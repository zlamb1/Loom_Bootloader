#ifndef LOOM_ERROR_H
#define LOOM_ERROR_H 1

#include "loom/compiler.h"
#include "loom/types.h"

#define LOOM_ERR_NONE            0
#define LOOM_ERR_BAD_ARG         1
#define LOOM_ERR_ALLOC           2
#define LOOM_ERR_OVERFLOW        3
#define LOOM_ERR_RANGE           4
#define LOOM_ERR_IO              5
#define LOOM_ERR_BAD_BLOCK_SIZE  6
#define LOOM_ERR_BAD_ELF_EHDR    7
#define LOOM_ERR_BAD_ELF_PHDR    8
#define LOOM_ERR_BAD_ELF_SHDR    9
#define LOOM_ERR_BAD_ELF_STRTAB  10
#define LOOM_ERR_BAD_ELF_REL     11
#define LOOM_ERR_BAD_MODULE      12
#define LOOM_ERR_BAD_PART_SCHEME 13
#define LOOM_ERR_BAD_FS          14
#define LOOM_ERR_HOOK            15
#define LOOM_ERR_PLATFORM        16
#define LOOM_ERR_NOTDIR          17
#define LOOM_ERR_ISDIR           18
#define LOOM_ERR_NOENT           19

#define LOOM_ERROR(ERROR, ...)                                                \
  do                                                                          \
    {                                                                         \
      loomErrorFmt ((ERROR), ##__VA_ARGS__);                                  \
      return -1;                                                              \
    }                                                                         \
  while (0)

typedef uint loom_error;

extern loom_error export_var (loom_errno);

loom_error printf_func (2, 3) export (loomErrorFmt) (loom_error,
                                                     const char *fmt, ...);
const char *export (loomErrorGet) (void);
void export (loomErrorClear) (void);

void noreturn printf_func (1, 2) export (loomPanic) (const char *fmt, ...);

static inline const char *force_inline
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
    case LOOM_ERR_BAD_FS:
      return "Bad filesystem";
    case LOOM_ERR_HOOK:
      return "Bad hook";
    case LOOM_ERR_PLATFORM:
      return "Platform error";
    case LOOM_ERR_NOTDIR:
      return "Not a directory";
    case LOOM_ERR_ISDIR:
      return "Is directory";
    case LOOM_ERR_NOENT:
      return "No entry found";
    default:
      return "Unknown error";
    }
}

static inline loom_error force_inline
loomError (loom_error error)
{
  loomErrorFmt (error, "%s", loomStringError (error));
  return error;
}

#endif