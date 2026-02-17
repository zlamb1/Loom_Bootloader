#include "loom/disk.h"
#include "loom/assert.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/string.h"

loom_list_t loom_disks = LOOM_LIST_HEAD (loom_disks);

void
loom_disk_register (loom_disk_t *disk)
{
  loom_assert (disk != NULL);
  loom_list_prepend (&loom_disks, &disk->node);
}

loom_error_t
loom_disk_read (loom_disk_t *disk, loom_usize_t offset, loom_usize_t count,
                char *buf)
{
  loom_error_t error = LOOM_ERR_NONE;
  loom_usize_t bpb, block, rem;
  char *bounce = NULL;

  if (!disk)
    return LOOM_ERR_BAD_ARG;

  if (!count)
    return LOOM_ERR_NONE;

  if (!buf)
    return LOOM_ERR_BAD_ARG;

  bpb = disk->bpb;

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

      if ((error = disk->read (disk, block, 1, bounce)))
        goto done;

      loom_memcpy (buf, bounce + rem, read);

      count -= read;
      buf += read;

      ++block;
    }

  if (count)
    {
      loom_usize_t blocks = count / bpb, bytes;

      if (loom_mul (blocks, bpb, &bytes))
        {
          error = LOOM_ERR_OVERFLOW;
          goto done;
        }

      if ((error = disk->read (disk, block, blocks, buf)))
        goto done;

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

      if ((error = disk->read (disk, block, 1, bounce)))
        goto done;

      loom_memcpy (buf, bounce, count);
    }

done:
  loom_free (bounce);
  return error;
}