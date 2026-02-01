#ifndef LOOM_DISK_H
#define LOOM_DISK_H 1

#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/types.h"

typedef struct loom_disk_t
{
  loom_error_t (*read) (struct loom_disk_t *, loom_usize_t, loom_usize_t,
                        char *);

  loom_usize_t bpb; // Bytes per block.
  loom_usize_t blocks;
  void *data;
  struct loom_disk_t *next;
} loom_disk_t;

extern loom_disk_t *LOOM_EXPORT_VAR (loom_disks);

void LOOM_EXPORT (loom_disk_register) (loom_disk_t *disk);

loom_error_t LOOM_EXPORT (loom_disk_read) (loom_disk_t *disk,
                                           loom_usize_t offset,
                                           loom_usize_t count, char *buf);

#endif