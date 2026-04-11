#include "loom/block_dev.h"
#include "loom/assert.h"
#include "loom/error.h"
#include "loom/fs.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/print.h"
#include "loom/string.h"
#include "loom/types.h"

loom_list loom_block_devs = LOOM_LIST_HEAD (loom_block_devs);

#define BLOCK_UNSET U64_MAX

static void
linkMruHead (block_dev_cache *bd_cache, block_dev_cache_entry *entry)
{
  auto mru = bd_cache->mru;

  if (mru == null)
    entry->prev = entry->next = entry;
  else
    {
      entry->prev = mru->prev;
      entry->next = mru;

      mru->prev->next = entry;
      mru->prev = entry;
    }

  bd_cache->mru = entry;
}

int
loomBlockDevCacheInit (block_dev_cache *bd_cache, usize count,
                       usize block_size)
{
  loomAssert (bd_cache != null);
  loomAssert (count > 0);
  loomAssert (block_size > 0);

  bd_cache->count = 0;
  bd_cache->mru = null;

  usize table_size, data_size;

  if (loomMul (sizeof (block_dev_cache_entry), count, &table_size))
    goto out1;

  bd_cache->table = loomAlloc (table_size);
  if (bd_cache->table == null)
    goto out1;

  if (loomMul (block_size, count, &data_size))
    goto out2;

  bd_cache->data = loomAlloc (data_size);
  if (bd_cache->data == null)
    goto out2;

  for (usize i = 0; i < count; ++i)
    {
      auto entry = bd_cache->table + i;
      entry->block = BLOCK_UNSET;

      linkMruHead (bd_cache, entry);
    }

  bd_cache->count = count;
  bd_cache->block_size = block_size;

  return 0;

out2:
  loomFree (bd_cache->table);
out1:
  bd_cache->table = bd_cache->data = null;
  return -1;
}

void
loomBlockDevCacheInvalidate (block_dev_cache *bd_cache)
{
  loomAssert (bd_cache != null);
  loomAssert (bd_cache->table != null);
  loomAssert (bd_cache->mru != null);
  loomAssert (bd_cache->data != null);

  auto mru = bd_cache->mru;
  auto entry = mru;

  do
    {
      entry->block = BLOCK_UNSET;
      entry = entry->next;
    }
  while (entry != mru);
}

void *
loomBlockDevCacheLookup (block_dev_cache *bd_cache, u64 block)
{
  loomAssert (bd_cache != null);
  loomAssert (bd_cache->table != null);
  loomAssert (bd_cache->mru != null);
  loomAssert (bd_cache->data != null);

  if (block == BLOCK_UNSET)
    return null;

  auto mru = bd_cache->mru;
  auto entry = mru;

  do
    {
      if (entry->block == block)
        {
          auto entry_nr = entry - bd_cache->table;

          char *ptr = bd_cache->data;
          ptr += bd_cache->block_size * (usize) entry_nr;

          if (entry != mru)
            {
              // Unlink the entry.
              entry->prev->next = entry->next;
              entry->next->prev = entry->prev;

              // Mark this entry most recently used.
              linkMruHead (bd_cache, entry);
            }

          return ptr;
        }

      entry = entry->next;
    }
  while (entry != bd_cache->mru);

  return null;
}

static inline void *
getCacheEntryData (block_dev_cache *bd_cache, block_dev_cache_entry *entry)
{
  auto entry_nr = entry - bd_cache->table;
  char *ptr = bd_cache->data;
  ptr += bd_cache->block_size * (usize) entry_nr;
  return ptr;
}

static inline void
cacheCopyToEntry (block_dev_cache *bd_cache, block_dev_cache_entry *entry,
                  void *data)
{
  loomMemCopy (getCacheEntryData (bd_cache, entry), data,
               bd_cache->block_size);
}

void
loomBlockDevCacheStore (block_dev_cache *bd_cache, u64 block, void *data)
{
  loomAssert (bd_cache != null);
  loomAssert (bd_cache->table != null);
  loomAssert (bd_cache->mru != null);
  loomAssert (bd_cache->data != null);

  auto mru = bd_cache->mru;

  auto to_evict = mru->prev;
  to_evict->block = block;

  cacheCopyToEntry (bd_cache, to_evict, data);

  // Move to the front. This is equivalent to shifting
  // the circular list to the right one by one.
  bd_cache->mru = to_evict;
}

void
loomBlockDevCacheFree (block_dev_cache *bd_cache)
{
  loomAssert (bd_cache != null);
  loomAssert (bd_cache->table != null);
  loomAssert (bd_cache->data != null);

  loomFree (bd_cache->table);
  loomFree (bd_cache->data);
}

void
loomBlockDevRegister (loom_block_dev *block_dev)
{
  loomAssert (block_dev != NULL);
  loomAssert (block_dev->readv != NULL);
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
  return loomBlockDevUncachedRead (block_dev, offset, size, buf);
}

loom_error
loomBlockDevUncachedRead (loom_block_dev *block_dev, usize offset, usize size,
                          char *buf)
{
  loom_error error = LOOM_ERR_NONE;
  loom_io_req io_reqs[3];

  usize count = 0;

  loomAssert (block_dev != null);
  loomAssert (block_dev->scratch != null);
  loomAssert (block_dev->readv != null);
  loomAssert (block_dev->block_size > 0);

  loomAssert (!size || buf != null);

  auto block_size = block_dev->block_size;
  auto block_offset = offset % block_size;
  auto block = offset / block_size;

  char *scratch = block_dev->scratch;

  usize copy_first = 0, copy_last = 0;

  char *mbuf = buf;

  if (block_offset)
    {
      copy_first = loomMin (block_size - block_offset, size);

      io_reqs[count++] = (loom_io_req) {
        .block = block,
        .count = 1,
        .buf = scratch,
      };

      size -= copy_first;
      block += 1;

      mbuf += copy_first;
    }

  if (size >= block_size)
    {
      auto blocks = size / block_size;
      auto nbytes = blocks * block_size;

      io_reqs[count++] = (loom_io_req) {
        .block = block,
        .count = blocks,
        .buf = mbuf,
      };

      size -= nbytes;
      block += blocks;

      mbuf += nbytes;
    }

  if (size)
    {
      loomAssert (size < block_size);

      io_reqs[count++] = (loom_io_req) {
        .block = block,
        .count = 1,
        .buf = scratch + block_size,
      };

      copy_last = size;
    }

  if (count)
    {
      error = block_dev->readv (block_dev, count, io_reqs);
      if (error)
        return error;
    }

  if (copy_first)
    loomMemCopy (buf, scratch + block_offset, copy_first);

  if (copy_last)
    loomMemCopy (mbuf, scratch + block_size, copy_last);

  return LOOM_ERR_NONE;
}

typedef struct
{
  usize count;
  bool log;
} partition_hook_ctx;

#ifndef LOOM_UTIL
static int
partitionHook (loom_block_dev *parent, loom_partition *partition, void *p)
{
  partition_hook_ctx *ctx = p;
  (void) parent;

  ctx->count += 1;

  loom_partition *n = loomAlloc (sizeof (*n));

  if (n == NULL)
    return -1;

  usize block_size = partition->base.block_size;

  if (ctx->log)
    loomLogLn ("Found partition [offset=0x%lx,size=0x%lx]",
               (ulong) (partition->offset * block_size),
               (ulong) (partition->base.blocks * block_size));

  loomMemCopy (n, partition, sizeof (*n));
  // Note: Make sure to update data to point to the new allocation.
  n->base.data = n;

  loomBlockDevRegister (&n->base);

  return 0;
}
#endif

void
loomBlockDevProbe (loom_block_dev *block_dev, bool force, unused bool log)
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
  partition_hook_ctx ctx = { .log = log };

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
        if (log)
          loomLogLn ("Found filesystem [type=%s]", fs_type->name);
        fs->parent = block_dev;
        fs->fs_type = fs_type;
        loomFsRegister (fs);
        return;
      }

    loomErrorClear ();
  }

#endif
}