#ifndef LOOM_BLOCK_DEV_H
#define LOOM_BLOCK_DEV_H 1

#include "loom/error.h"
#include "loom/list.h"

struct loom_block_dev_t;

typedef loom_error_t (*block_dev_read) (struct loom_block_dev_t *,
                                        loom_usize_t, loom_usize_t, char *);

typedef struct loom_block_dev_t
{
  struct loom_block_dev_t *parent;
  block_dev_read read;
  loom_usize_t blocksz;
  loom_usize_t blocks;
#define LOOM_BLOCK_DEVICE_FLAG_PROBED (1 << 0)
  loom_uint8_t flags;
  void *data;
  loom_list_t children, child_node, node;
} loom_block_dev_t;

extern loom_list_t LOOM_EXPORT_VAR (loom_block_devs);

typedef struct
{
  loom_block_dev_t *parent;
  block_dev_read read;
  loom_usize_t blocksz;
  loom_usize_t blocks;
  void *data;
} loom_block_dev_init_t;

static inline void
loom_block_dev_init (loom_block_dev_t *block_dev, loom_block_dev_init_t *init)
{
  loom_block_dev_init_t zero = { .parent = NULL, .read = NULL, .data = NULL };

  if (init == NULL)
    init = &zero;

  block_dev->parent = init->parent;
  block_dev->read = init->read;
  block_dev->blocksz = init->blocksz;
  block_dev->blocks = init->blocks;
  block_dev->flags = 0;
  block_dev->data = init->data;
  block_dev->children = LOOM_LIST_HEAD (block_dev->children);
  block_dev->child_node = LOOM_LIST_HEAD (block_dev->child_node);
}

void LOOM_EXPORT (loom_block_dev_register) (loom_block_dev_t *block_dev);
void LOOM_EXPORT (loom_block_dev_unregister) (loom_block_dev_t *block_dev);

loom_error_t LOOM_EXPORT (loom_block_dev_read) (loom_block_dev_t *block_dev,
                                                loom_usize_t offset,
                                                loom_usize_t size, char *buf);

void LOOM_EXPORT (loom_block_dev_probe) (loom_block_dev_t *block_dev,
                                         loom_bool_t force);

#endif