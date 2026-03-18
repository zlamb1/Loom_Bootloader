#ifndef LOOM_OSDEP_H
#define LOOM_OSDEP_H 1

#define LOOM_NOSTDINT
#include "loom/error.h"
#include "loom/types.h"

#define LOOM_O_RDONLY (1 << 0)
#define LOOM_O_WRONLY (1 << 1)
#define LOOM_O_RDWR   (1 << 2)
#define LOOM_O_APPEND (1 << 3)
#define LOOM_O_CREAT  (1 << 4)
#define LOOM_O_TRUNC  (1 << 5)

typedef int loom_file_t;

typedef struct
{
  loom_usize_t size;
} loom_file_meta_t;

typedef struct
{
  loom_usize_t size;
  void *buf;
} loom_slice_t;

loom_error_t loom_file_open (const char *path, uint oflags, loom_file_t *file);
loom_error_t loom_file_meta (loom_file_t file, loom_file_meta_t *meta);
loom_error_t loom_file_sync (loom_file_t file);
loom_error_t loom_file_close (loom_file_t file);

loom_error_t loom_file_read (loom_file_t file, void *buf, loom_usize_t nbytes);
loom_error_t loom_file_read_all (loom_file_t file, loom_slice_t *slice);

loom_error_t loom_file_write (loom_file_t file, void *buf,
                              loom_usize_t nbytes);
loom_error_t loom_file_write_exact (loom_file_t file, void *buf,
                                    loom_usize_t off, loom_usize_t nbytes);

#endif