#ifndef LOOM_ERROR_H
#define LOOM_ERROR_H 1

#include "compiler.h"
#include "types.h"

#define LOOM_ERR_NONE           0
#define LOOM_ERR_BAD_ARG        1
#define LOOM_ERR_ALLOC          2
#define LOOM_ERR_OVERFLOW       3
#define LOOM_ERR_RANGE          4
#define LOOM_ERR_IO             5
#define LOOM_ERR_BAD_BLOCK_SIZE 6
#define LOOM_ERR_BAD_ELF_HDR    7
#define LOOM_ERR_BAD_ELF_PHDR   8
#define LOOM_ERR_BAD_ELF_SHDR   9
#define LOOM_ERR_BAD_ELF_REL    10
#define LOOM_ERR_BAD_MODULE     11

#define LOOM_ERROR(COND, ERROR, ...)                                          \
  do                                                                          \
    {                                                                         \
      if (COND)                                                               \
        {                                                                     \
          loom_error ((ERROR), ##__VA_ARGS__);                                \
          return -1;                                                          \
        }                                                                     \
    }                                                                         \
  while (0)

#define LOOM_ERROR_GOTO(COND, LABEL, ERROR, ...)                              \
  do                                                                          \
    {                                                                         \
      if (COND)                                                               \
        {                                                                     \
          loom_error ((ERROR), ##__VA_ARGS__);                                \
          goto LABEL;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

typedef loom_uint8_t loom_error_t;

extern loom_error_t loom_errno;

void PRINTF (2, 3) EXPORT (loom_error) (loom_error_t, const char *fmt, ...);
const char *EXPORT (loom_error_get) (void);
void EXPORT (loom_error_clear) (void);

const char *EXPORT (loom_strerror) (loom_error_t);

void NORETURN PRINTF (1, 2) EXPORT (loom_panic) (const char *fmt, ...);

#endif