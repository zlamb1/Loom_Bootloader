#ifndef LOOM_PARTITION_H
#define LOOM_PARTITION_H 1

#include "loom/block_dev.h"
#include "loom/partition_scheme.h"

typedef struct loom_partition_t
{
  loom_block_dev_t base;
  loom_usize_t offset;
} loom_partition_t;

extern loom_list_t loom_partition_schemes;

loom_error_t loom_partition_read (loom_block_dev_t *block_dev,
                                  loom_usize_t block, loom_usize_t n,
                                  char *buf);

#endif