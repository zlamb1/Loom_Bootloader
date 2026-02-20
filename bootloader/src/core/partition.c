#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/math.h"

loom_error_t
loom_partition_read (loom_partition_t *p, loom_usize_t offset,
                     loom_usize_t count, char *buf)
{
  loom_disk_t *disk;
  loom_partition_t *curp = p;

  loom_usize_t block = 0, block_count, start;

  loom_assert (p != NULL);

  while (!curp->root)
    {
      if (loom_add (block, curp->start, &block))
        return LOOM_ERR_OVERFLOW;
      block += curp->start;
      curp = curp->parent;
    }

  if (loom_add (block, curp->start, &block))
    return LOOM_ERR_OVERFLOW;

  block += curp->start;
  disk = curp->disk;

  if (!disk->bpb)
    return LOOM_ERR_BAD_BLOCK_SIZE;

  if (loom_mul (block, disk->bpb, &start))
    return LOOM_ERR_OVERFLOW;

  if (loom_add (start, offset, &start))
    return LOOM_ERR_OVERFLOW;

  block_count = count / disk->bpb;
  if ((count % disk->bpb))
    {
      if (block_count == LOOM_USIZE_MAX)
        return LOOM_ERR_OVERFLOW;
      ++block_count;
    }

  if (block_count > p->length)
    return LOOM_ERR_RANGE;

  return loom_disk_read (disk, start, count, buf);
}