#include "loom/partition.h"
#include "loom/assert.h"
#include "loom/list.h"
#include "loom/math.h"

loom_list_t loom_partition_schemes = LOOM_LIST_HEAD (loom_partition_schemes);

void
loom_partition_scheme_register (loom_partition_scheme_t *partition_scheme)
{
  loom_assert (partition_scheme != NULL);
  loom_list_prepend (&loom_partition_schemes, &partition_scheme->node);
}

loom_error_t
loom_partition_read (loom_partition_t *p, loom_usize_t offset,
                     loom_usize_t count, char *buf)
{
  loom_disk_t *disk;
  loom_partition_t *curp = p;

  loom_usize_t block = 0, block_count, start;

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