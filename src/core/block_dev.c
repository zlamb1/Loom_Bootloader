#include "loom/block_dev.h"
#include "loom/assert.h"
#include "loom/error.h"
#include "loom/fs.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/string.h"
#include "loom/types.h"

loom_list loom_block_devs = LOOM_LIST_HEAD (loom_block_devs);

void
loomBlockDevRegister (loom_block_dev *block_dev)
{
  loomAssert (block_dev != NULL);
  loomAssert (block_dev->read != NULL);
  loomAssert (block_dev->block_size);

  loomListPrepend (&loom_block_devs, &block_dev->node);
}

void
loomBlockDevUnregister (loom_block_dev *block_dev)
{
  loomAssert (block_dev != NULL);

  loomListRemove (&block_dev->node);
}

loom_error
loomBlockDevRead (loom_block_dev *block_dev, usize offset, usize size,
                  char *buf)
{
  usize mod, block, blocks, block_size, disk_end;
  loom_error error = LOOM_ERR_NONE;

  char *bounce = NULL;

  loomAssert (block_dev != NULL);
  loomAssert (block_dev->read != NULL);

  if (!size)
    return LOOM_ERR_NONE;

  block_size = block_dev->block_size;
  loomAssert (block_size);

  if (loomAdd (offset, size - 1, &mod))
    return LOOM_ERR_OVERFLOW;

  if (loomMul (block_dev->blocks, block_size, &disk_end))
    // Note: Not all blocks are accessible as the disk is larger than the
    // (typically 32-bit) address space.
    disk_end = USIZE_MAX;

  if (mod >= disk_end)
    return LOOM_ERR_RANGE;

  block = offset / block_size;
  mod = offset % block_size;

  if (mod)
    {
      usize read = block_size - mod;

      if (read > size)
        read = size;

      if (size >= block_size)
        {
          // Note: Use the buffer for the first partial read if it can contain
          // at least one block.

          if ((error = block_dev->read (block_dev, block, 1, buf)))
            goto done;

          loomMemMove (buf, buf + mod, read);

          block += 1;
          size -= read;
          buf += read;
        }
      else
        {
          bounce = loomAlloc (block_size);

          if (bounce == NULL)
            {
              error = loom_errno;
              goto done;
            }

          if ((error = block_dev->read (block_dev, block, 1, bounce)))
            goto done;

          loomMemCopy (buf, bounce + mod, read);
          goto done;
        }
    }

  blocks = size / block_size;

  if (blocks)
    {
      usize read = blocks * block_size;

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
          bounce = loomAlloc (block_size);

          if (bounce == NULL)
            {
              error = loom_errno;
              goto done;
            }
        }

      if ((error = block_dev->read (block_dev, block, 1, bounce)))
        goto done;

      loomMemCopy (buf, bounce, size);
    }

done:
  loomFree (bounce);
  return error;
}

typedef struct
{
  usize count;
} partition_hook_ctx;

static used int
partitionHook (loom_block_dev *parent, loom_partition *partition, void *p)
{
  partition_hook_ctx *ctx = p;
  (void) parent;

  ctx->count += 1;

  loom_partition *n = loomAlloc (sizeof (*n));

  if (n == NULL)
    return -1;

  loomMemCopy (n, partition, sizeof (*n));
  loomBlockDevRegister (&n->base);

  return 0;
}

void
loomBlockDevProbe (loom_block_dev *block_dev, bool force)
{
  loom_partition_scheme *partition_scheme;
  loom_fs_type *fs_type;

  (void) partition_scheme;
  (void) fs_type;

  loomAssert (block_dev != NULL);

  if (force)
    block_dev->flags &= (u8) ~LOOM_BLOCK_DEVICE_FLAG_PROBED;

  if (block_dev->flags & LOOM_BLOCK_DEVICE_FLAG_PROBED)
    return;

  block_dev->flags |= LOOM_BLOCK_DEVICE_FLAG_PROBED;

#ifndef LOOM_UTIL
  partition_hook_ctx ctx = { 0 };

  loom_list_for_each_entry (&loom_partition_schemes, partition_scheme, node)
  {
    loomAssert (partition_scheme->iterate != NULL);
    partition_scheme->iterate (partition_scheme, block_dev, partitionHook,
                               &ctx);
    loomErrorClear ();
    if (ctx.count)
      return;
  }

  loom_list_for_each_entry (&loom_fs_types, fs_type, node)
  {
    loom_fs *fs;
    loomAssert (fs_type->probe != NULL);

    if ((fs = fs_type->probe (block_dev)) != NULL)
      {
        fs->parent = block_dev;
        fs->fs_type = fs_type;
        loomFsRegister (fs);
        return;
      }

    loomErrorClear ();
  }

#endif
}