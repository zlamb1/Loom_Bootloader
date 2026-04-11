#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/math.h"

loom_error
loomPartitionRead (loom_block_dev *block_dev, usize count,
                   loom_io_req io_reqs[])
{
  loom_block_dev *parent;
  loom_partition *partition;
  usize end;

  loomAssert (block_dev != NULL);
  loomAssert (block_dev->parent != NULL);
  loomAssert (block_dev->parent->readv != NULL);
  loomAssert (block_dev->data != NULL);

  parent = block_dev->parent;
  partition = block_dev->data;

  if (!count)
    return LOOM_ERR_NONE;

  for (usize i = 0; i < count; i++)
    {
      auto io_req = &io_reqs[i];

      if (!io_req->count)
        continue;

      if (loomAdd (io_req->block, io_req->count - 1, &end))
        return LOOM_ERR_OVERFLOW;

      if (end >= block_dev->blocks)
        return LOOM_ERR_RANGE;

      if (loomAdd (end, partition->offset, &end))
        return LOOM_ERR_OVERFLOW;

      if (end >= parent->blocks)
        return LOOM_ERR_RANGE;

      io_req->block += partition->offset;
    }

  return parent->readv (parent, count, io_reqs);
}