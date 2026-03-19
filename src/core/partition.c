#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/math.h"

loom_error
loomPartitionRead (loom_block_dev *block_dev, usize block, usize n, char *buf)
{
  loom_block_dev *parent;
  loom_partition *partition;
  usize end;

  loomAssert (block_dev != NULL);
  loomAssert (block_dev->parent != NULL);
  loomAssert (block_dev->parent->read != NULL);
  loomAssert (block_dev->data != NULL);

  parent = block_dev->parent;
  partition = block_dev->data;

  if (!n)
    return LOOM_ERR_NONE;

  if (loomAdd (block, n - 1, &end))
    return LOOM_ERR_OVERFLOW;

  if (end >= block_dev->blocks)
    return LOOM_ERR_RANGE;

  if (loomAdd (end, partition->offset, &end))
    return LOOM_ERR_OVERFLOW;

  if (end >= parent->blocks)
    return LOOM_ERR_RANGE;

  block += partition->offset;

  return parent->read (parent, block, n, buf);
}