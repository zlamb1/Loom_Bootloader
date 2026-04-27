#ifndef LOOM_BLOCK_DEV_H
#define LOOM_BLOCK_DEV_H 1

#include "loom/error.h"
#include "loom/list.h"
#include "loom/mm.h"

struct loom_block_dev;

typedef struct loom_io_req
{
  u64 block;
  u64 count;
  void *buf;
  struct loom_io_req *next;
} loom_io_req;

typedef loom_error (*block_dev_readv) (struct loom_block_dev *, loom_io_req *);

typedef struct block_dev_cache_entry
{
  u64 block;
  struct block_dev_cache_entry *prev;
  struct block_dev_cache_entry *next;
} block_dev_cache_entry;

typedef struct
{
  usize count;
  usize block_size;
  block_dev_cache_entry *table; // for lookup
  block_dev_cache_entry *mru;   // head is mru; tail is lru
  void *data;                   // cached block data
  loom_io_req *io_reqs;         // cap=count; useful for batching requests
} loom_block_dev_cache;

typedef struct loom_block_dev
{
  struct loom_block_dev *parent;
  block_dev_readv readv;
  loom_block_dev_cache *cache;
  void *scratch;
  usize block_size;
  usize blocks;
#ifdef LOOM_DEBUG
  usize read_count;
#endif
#define LOOM_BLOCK_DEVICE_FLAG_PROBED (1 << 0)
  byte flags;
  void *data;
  loom_list children, child_node, node;
} loom_block_dev;

extern loom_list export_var (loom_block_devs);

typedef struct
{
  loom_block_dev *parent;
  block_dev_readv readv;
  usize block_size;
  usize blocks;
  usize cache_count;
  void *data;
} loom_block_dev_init_t;

int export (loomBlockDevCacheInit) (loom_block_dev_cache *cache, usize count,
                                    usize block_size);
void export (loomBlockDevCacheInvalidate) (loom_block_dev_cache *cache);
void *export (loomBlockDevCacheLookup) (loom_block_dev_cache *cache,
                                        u64 block);
void export (loomBlockDevCacheStore) (loom_block_dev_cache *cache, u64 block,
                                      void *data);
void export (loomBlockDevCacheFree) (loom_block_dev_cache *cache);

static inline int
loomBlockDevInit (loom_block_dev *block_dev, loom_block_dev_init_t *init)
{
  loom_block_dev_init_t zero = { .parent = NULL, .readv = NULL, .data = NULL };

  if (init == NULL)
    init = &zero;

  auto block_size = block_dev->block_size;

  block_dev->scratch = block_dev->cache = null;

  block_dev->parent = init->parent;
  block_dev->readv = init->readv;

  if (init->cache_count > 0)
    {
      block_dev->cache = loomAlloc (sizeof (loom_block_dev_cache));

      if (block_dev->cache == null)
        return -1;

      if (loomBlockDevCacheInit (block_dev->cache, init->cache_count,
                                 block_size))
        goto out;
    }
  else
    block_dev->cache = null;

  block_dev->scratch = loomAlloc (block_size * 2);

  if (block_dev->scratch == null)
    goto out;

  block_dev->block_size = init->block_size;
  block_dev->blocks = init->blocks;
#ifdef LOOM_DEBUG
  block_dev->read_count = 0;
#endif
  block_dev->flags = 0;
  block_dev->data = init->data;
  block_dev->children = LOOM_LIST_HEAD (block_dev->children);
  block_dev->child_node = LOOM_LIST_HEAD (block_dev->child_node);

  return 0;

out:
  if (block_dev->cache != null)
    {
      loomBlockDevCacheFree (block_dev->cache);
      loomFree (block_dev->cache);
    }

  loomFree (block_dev->scratch);

  return -1;
}

void export (loomBlockDevRegister) (loom_block_dev *block_dev);
void export (loomBlockDevUnregister) (loom_block_dev *block_dev);

loom_error export (loomBlockDevCachedRead) (loom_block_dev *block_dev,
                                            usize offset, usize size,
                                            char *buf);
loom_error export (loomBlockDevRead) (loom_block_dev *block_dev, usize offset,
                                      usize size, char *buf);

void export (loomBlockDevProbe) (loom_block_dev *block_dev, bool force,
                                 bool log);

#endif