#ifndef LOOM_PARTITION_SCHEME_H
#define LOOM_PARTITION_SCHEME_H 1

#include "loom/block_dev.h"
#include "loom/compiler.h"
#include "loom/list.h"

struct loom_partition_t;

typedef int (loom_partition_scheme_hook_t) (loom_block_dev_t *,
                                            struct loom_partition_t *);

typedef struct loom_partition_scheme_t
{
  int (*iterate) (struct loom_partition_scheme_t *, loom_block_dev_t *,
                  loom_partition_scheme_hook_t);
  void *data;
  loom_list_t node;
} loom_partition_scheme_t;

extern loom_list_t LOOM_EXPORT_VAR (loom_partition_schemes);

void LOOM_EXPORT (loom_partition_scheme_register) (loom_partition_scheme_t *);
void
    LOOM_EXPORT (loom_partition_scheme_unregister) (loom_partition_scheme_t *);

#endif