#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/math.h"

loom_error
loomPartitionRead (loom_block_dev *block_dev, loom_io_req *io_reqs)
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

  if (io_reqs == null)
    return LOOM_ERR_NONE;

  auto current = io_reqs;

  while (current != null)
    {
      auto block = current->block;
      auto count = current->count;

      if (!count)
        continue;

      if (loomAdd (block, count - 1, &end))
        return LOOM_ERR_OVERFLOW;

      if (end >= block_dev->blocks)
        return LOOM_ERR_RANGE;

      if (loomAdd (end, partition->offset, &end))
        return LOOM_ERR_OVERFLOW;

      if (end >= parent->blocks)
        return LOOM_ERR_RANGE;

      /* Move requests into the linear address space of the parent block
       * device. */
      current->block += partition->offset;

      current = current->next;
    }

  /* Forward to parent device. */
  return parent->readv (parent, io_reqs);
}