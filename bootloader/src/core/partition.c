#include "loom/partition.h"

loom_partition_scheme_t *loom_partition_scheme_list = NULL;
loom_partition_t *loom_partition_list = NULL;

void
loom_register_partition_scheme (loom_partition_scheme_t *partition_scheme)
{
  partition_scheme->next = loom_partition_schemes;
  loom_partition_schemes = partition_scheme;
}

void
loom_register_partition (loom_partition_t *partition)
{
  partition->next = loom_partitions;
  loom_partitions = partition;
}

loom_error_t
loom_partition_read (loom_partition_t *p, loom_usize_t offset,
                     loom_usize_t count, char *buf)
{
  loom_disk_dev_t *disk_dev;
  loom_partition_t *curp = p;

  loom_usize_t block = 0, block_count, start;

  while (!curp->root)
    {
      if (block > LOOM_USIZE_MAX - curp->start)
        return LOOM_ERR_OVERFLOW;
      block += curp->start;
      curp = curp->parent;
    }

  if (block > LOOM_USIZE_MAX - curp->start)
    return LOOM_ERR_OVERFLOW;

  block += curp->start;
  disk_dev = curp->disk_dev;

  if (!disk_dev->bpb)
    return LOOM_ERR_BAD_BLOCK_SIZE;

  if (block > LOOM_USIZE_MAX / disk_dev->bpb)
    return LOOM_ERR_OVERFLOW;

  start = block * disk_dev->bpb;
  if (start > LOOM_USIZE_MAX - offset)
    return LOOM_ERR_OVERFLOW;

  block_count = count / disk_dev->bpb;
  if ((count % disk_dev->bpb))
    {
      if (block_count == LOOM_USIZE_MAX)
        return LOOM_ERR_OVERFLOW;
      ++block_count;
    }

  if (block_count > p->length)
    return LOOM_ERR_RANGE;

  return loom_disk_read (disk_dev, start + offset, count, buf);
}