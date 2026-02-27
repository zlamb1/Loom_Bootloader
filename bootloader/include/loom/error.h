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

#define LOOM_ERROR(ERROR, ...)                                                \
  do                                                                          \
    {                                                                         \
      loom_error ((ERROR), ##__VA_ARGS__);                                    \
      return -1;                                                              \
    }                                                                         \
  while (0)

typedef loom_uint8_t loom_error_t;

extern loom_error_t LOOM_EXPORT_VAR (loom_errno);

loom_error_t LOOM_PRINTF (2, 3)
    LOOM_EXPORT (loom_error) (loom_error_t, const char *fmt, ...);
const char *LOOM_EXPORT (loom_error_get) (void);
void LOOM_EXPORT (loom_error_clear) (void);

const char *LOOM_EXPORT (loom_strerror) (loom_error_t);

void LOOM_NORETURN LOOM_PRINTF (1, 2)
    LOOM_EXPORT (loom_panic) (const char *fmt, ...);

#endif