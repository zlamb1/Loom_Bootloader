#ifndef LOOM_BLOCK_DEV_H
#define LOOM_BLOCK_DEV_H 1

#include "loom/error.h"
#include "loom/list.h"

struct loom_block_dev;

typedef loom_error (*block_dev_read) (struct loom_block_dev *, usize, usize,
                                      char *);

typedef struct loom_block_dev
{
  struct loom_block_dev *parent;
  block_dev_read read;
  usize block_size;
  usize blocks;
#define LOOM_BLOCK_DEVICE_FLAG_PROBED (1 << 0)
  byte flags;
  void *data;
  loom_list children, child_node, node;
} loom_block_dev;

extern loom_list export_var (loom_block_devs);

typedef struct
{
  loom_block_dev *parent;
  block_dev_read read;
  usize block_size;
  usize blocks;
  void *data;
} loom_block_dev_init_t;

static inline void
loom_block_dev_init (loom_block_dev *block_dev, loom_block_dev_init_t *init)
{
  loom_block_dev_init_t zero = { .parent = NULL, .read = NULL, .data = NULL };

  if (init == NULL)
    init = &zero;

  block_dev->parent = init->parent;
  block_dev->read = init->read;
  block_dev->block_size = init->block_size;
  block_dev->blocks = init->blocks;
  block_dev->flags = 0;
  block_dev->data = init->data;
  block_dev->children = LOOM_LIST_HEAD (block_dev->children);
  block_dev->child_node = LOOM_LIST_HEAD (block_dev->child_node);
}

void export (loom_block_dev_register) (loom_block_dev *block_dev);
void export (loom_block_dev_unregister) (loom_block_dev *block_dev);

loom_error export (loom_block_dev_read) (loom_block_dev *block_dev,
                                         usize offset, usize size, char *buf);

void export (loom_block_dev_probe) (loom_block_dev *block_dev, bool force);

#endif