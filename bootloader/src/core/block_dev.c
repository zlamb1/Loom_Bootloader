#include "loom/block_dev.h"
#include "loom/assert.h"
#include "loom/error.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/string.h"
#include "loom/types.h"

loom_list_t loom_block_devs = LOOM_LIST_HEAD (loom_block_devs);

void
loom_block_dev_register (loom_block_dev_t *block_dev)
{
  loom_assert (block_dev != NULL);
  loom_assert (block_dev->read != NULL);
  loom_assert (block_dev->blocksz);

  loom_list_prepend (&loom_block_devs, &block_dev->node);
}

void
loom_block_dev_unregister (loom_block_dev_t *block_dev)
{
  loom_assert (block_dev != NULL);

  loom_list_remove (&block_dev->node);
}

loom_error_t
loom_block_dev_read (loom_block_dev_t *block_dev, loom_usize_t offset,
                     loom_usize_t size, char *buf)
{
  loom_usize_t mod, block, blocks, blocksz, disk_end;
  loom_error_t error = LOOM_ERR_NONE;

  char *bounce = NULL;

  loom_assert (block_dev != NULL);
  loom_assert (block_dev->read != NULL);

  if (!size)
    return LOOM_ERR_NONE;

  blocksz = block_dev->blocksz;
  loom_assert (blocksz);

  if (loom_add (offset, size - 1, &mod))
    return LOOM_ERR_OVERFLOW;

  if (loom_mul (block_dev->blocks, blocksz, &disk_end))
    // Note: Not all blocks are accessible as the disk is larger than the
    // (typically 32-bit) address space.
    disk_end = LOOM_USIZE_MAX;

  if (mod >= disk_end)
    return LOOM_ERR_RANGE;

  block = offset / blocksz;
  mod = offset % blocksz;

  if (mod)
    {
      loom_usize_t read = blocksz - mod;

      if (read > size)
        read = size;

      if (size >= blocksz)
        {
          // Note: Use the buffer for the first partial read if it can contain
          // at least one block.

          if ((error = block_dev->read (block_dev, block, 1, buf)))
            goto done;

          loom_memmove (buf, buf + mod, read);

          block += 1;
          size -= read;
          buf += read;
        }
      else
        {
          bounce = loom_malloc (blocksz);

          if (bounce == NULL)
            {
              error = loom_errno;
              goto done;
            }

          if ((error = block_dev->read (block_dev, block, 1, bounce)))
            goto done;

          loom_memcpy (buf, bounce + mod, read);
          goto done;
        }
    }

  blocks = size / blocksz;

  if (blocks)
    {
      loom_usize_t read = blocks * blocksz;

      if ((error = block_dev->read (block_dev, block, blocks, buf)))
        goto done;

      block += blocks;
      size -= read;
      buf += read;
    }

  if (size)
    {
      if (bounce == NULL)
        {
          bounce = loom_malloc (blocksz);

          if (bounce == NULL)
            {
              error = loom_errno;
              goto done;
            }
        }

      if ((error = block_dev->read (block_dev, block, 1, bounce)))
        goto done;

      loom_memcpy (buf, bounce, size);
    }

done:
  loom_free (bounce);
  return error;
}

typedef struct
{
  loom_usize_t count;
} partition_hook_ctx_t;

static int
partition_hook (loom_block_dev_t *parent, loom_partition_t *partition, void *p)
{
  partition_hook_ctx_t *ctx = p;
  (void) parent;

  ctx->count += 1;

  loom_partition_t *n = loom_malloc (sizeof (*n));

  if (n == NULL)
    return -1;

  loom_memcpy (n, partition, sizeof (*n));
  loom_block_dev_register (&n->base);

  return 0;
}

void
loom_block_dev_probe (loom_block_dev_t *block_dev, loom_bool_t force)
{
  loom_partition_scheme_t *partition_scheme;

  loom_assert (block_dev != NULL);

  if (force)
    block_dev->flags &= (loom_uint8_t) ~LOOM_BLOCK_DEVICE_FLAG_PROBED;

  if (block_dev->flags & LOOM_BLOCK_DEVICE_FLAG_PROBED)
    return;

  block_dev->flags |= LOOM_BLOCK_DEVICE_FLAG_PROBED;

  partition_hook_ctx_t ctx = { 0 };

  loom_list_for_each_entry (&loom_partition_schemes, partition_scheme, node)
  {
    loom_assert (partition_scheme->iterate != NULL);
    partition_scheme->iterate (partition_scheme, block_dev, partition_hook,
                               &ctx);
    loom_error_clear ();
    if (ctx.count)
      break;
  }
}