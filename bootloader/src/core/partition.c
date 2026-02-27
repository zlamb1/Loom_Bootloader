#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/math.h"

loom_error_t
loom_partition_read (loom_block_dev_t *block_dev, loom_usize_t block,
                     loom_usize_t n, char *buf)
{
  loom_block_dev_t *parent;
  loom_partition_t *partition;
  loom_usize_t end;

  loom_assert (block_dev != NULL);
  loom_assert (block_dev->parent != NULL);
  loom_assert (block_dev->parent->read != NULL);
  loom_assert (block_dev->data != NULL);

  parent = block_dev->parent;
  partition = block_dev->data;

  if (!n)
    return LOOM_ERR_NONE;

  if (loom_add (block, n - 1, &end))
    return LOOM_ERR_OVERFLOW;

  if (end >= block_dev->blocks)
    return LOOM_ERR_RANGE;

  if (loom_add (end, partition->offset, &end))
    return LOOM_ERR_OVERFLOW;

  if (end >= parent->blocks)
    return LOOM_ERR_RANGE;

  block += partition->offset;

  return parent->read (parent, block, n, buf);
}