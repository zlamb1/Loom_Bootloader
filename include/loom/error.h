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
#define LOOM_ERR_HOOK            14
#define LOOM_ERR_PLATFORM        15

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

const char *export (loomStringError) (loom_error);

void noreturn printf_func (1, 2) export (loomPanic) (const char *fmt, ...);

static inline loom_error force_inline
loomError (loom_error error)
{
  loomErrorFmt (error, "%s", loomStringError (error));
  return error;
}

#endif