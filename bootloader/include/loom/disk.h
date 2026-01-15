#ifndef LOOM_DISK_H
#define LOOM_DISK_H 1

#include "compiler.h"
#include "error.h"
#include "types.h"

typedef struct loom_disk_dev_t
{
  loom_usize_t bpb; // Bytes per block.
  loom_usize_t blocks;
  void *data;
  struct loom_disk_dev_t *next;

  loom_error_t (*read) (struct loom_disk_dev_t *, loom_usize_t, loom_usize_t,
                        char *);
} loom_disk_dev_t;

extern loom_disk_dev_t *loom_disks;

void EXPORT (loom_register_disk_dev) (loom_disk_dev_t *dev);

loom_error_t EXPORT (loom_disk_read) (loom_disk_dev_t *disk_dev,
                                      loom_usize_t offset, loom_usize_t count,
                                      char *buf);

#endif