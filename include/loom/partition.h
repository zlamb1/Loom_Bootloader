#ifndef LOOM_PARTITION_H
#define LOOM_PARTITION_H 1

#include "loom/assert.h"
#include "loom/block_dev.h"
#include "loom/partition_scheme.h"

typedef struct loom_partition
{
  loom_block_dev base;
  usize offset;
} loom_partition;

extern loom_list loom_partition_schemes;

loom_error export (loomPartitionRead) (loom_block_dev *block_dev,
                                       loom_io_req *io_reqs);

static inline int
loomPartitionInit (loom_partition *partition, loom_block_dev *parent,
                   usize offset, usize blocks)
{
  loomAssert (partition != NULL);
  loomAssert (parent != NULL);

  loom_block_dev_init_t init = {
    .parent = parent,
    .readv = loomPartitionRead,
    .block_size = parent->block_size,
    .blocks = blocks,
    .data = partition,
  };

  if (loomBlockDevInit (&partition->base, &init))
    return -1;

  loomListAdd (&parent->children, &partition->base.child_node);
  partition->offset = offset;

  return 0;
}

static inline void
loomPartitionDeinit (loom_partition *partition)
{
  loomAssert (partition != NULL);
  loomAssert (partition->base.parent != NULL);

  loomListRemove (&partition->base.child_node);
  loomListRemove (&partition->base.node);
}

#endif