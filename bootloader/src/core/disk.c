#include "loom/disk.h"
#include "loom/mm.h"
#include "loom/string.h"

loom_disk_dev_t *loom_disks = NULL;

void
loom_register_disk_dev (loom_disk_dev_t *dev)
{
  dev->next = loom_disks;
  loom_disks = dev;
}

loom_error_t
loom_disk_read (loom_disk_dev_t *disk_dev, loom_usize_t offset,
                loom_usize_t count, char *buf)
{
  loom_error_t error = LOOM_ERR_NONE;
  loom_usize_t bpb, block, rem;
  char *bounce = NULL;

  if (!disk_dev)
    return LOOM_ERR_BAD_ARG;

  if (!count)
    return LOOM_ERR_NONE;

  if (!buf)
    return LOOM_ERR_BAD_ARG;

  bpb = disk_dev->bpb;

  if (!bpb)
    return LOOM_ERR_BAD_BLOCK_SIZE;

  block = offset / bpb;
  rem = offset % bpb;

  if (rem)
    {
      loom_usize_t read = bpb - rem;
      if (read > count)
        read = count;

      bounce = loom_malloc (bpb);
      if (!bounce)
        return LOOM_ERR_ALLOC;

      if ((error = disk_dev->read (disk_dev, block, 1, bounce)))
        goto done;

      loom_memcpy (buf, bounce + rem, read);

      count -= read;
      buf += read;

      ++block;
    }

  if (count)
    {
      loom_usize_t blocks = count / bpb, bytes;

      if (blocks > LOOM_USIZE_MAX / bpb)
        {
          error = LOOM_ERR_OVERFLOW;
          goto done;
        }

      if ((error = disk_dev->read (disk_dev, block, blocks, buf)))
        goto done;

      bytes = blocks * bpb;
      count -= bytes;
      buf += bytes;

      block += blocks;
    }

  if (count)
    {
      if (count >= bpb)
        loom_panic ("loom_disk_read");

      if (!bounce)
        {
          bounce = loom_malloc (bpb);
          if (!bounce)
            {
              error = LOOM_ERR_ALLOC;
              goto done;
            }
        }

      if ((error = disk_dev->read (disk_dev, block, 1, bounce)))
        goto done;

      loom_memcpy (buf, bounce, count);
    }

done:
  loom_free (bounce);
  return error;
}