#ifndef LOOM_PARTITION_H
#define LOOM_PARTITION_H 1

#include "loom/assert.h"
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

static inline void
loom_partition_init (loom_partition_t *partition, loom_block_dev_t *parent,
                     loom_usize_t offset, loom_usize_t blocks)
{
  loom_assert (partition != NULL);
  loom_assert (parent != NULL);

  loom_block_dev_init_t init = {
    .parent = parent,
    .read = loom_partition_read,
    .blocksz = parent->blocksz,
    .blocks = blocks,
    .data = partition,
  };

  loom_block_dev_init (&partition->base, &init);
  partition->offset = offset;
}

#endif