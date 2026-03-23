#ifndef LOOM_PARTITION_SCHEME_H
#define LOOM_PARTITION_SCHEME_H 1

#include "loom/block_dev.h"
#include "loom/compiler.h"
#include "loom/list.h"

struct loom_partition;

typedef int (loom_partition_scheme_hook) (loom_block_dev *,
                                          struct loom_partition *, void *);

typedef struct loom_partition_scheme
{
  int (*iterate) (struct loom_partition_scheme *, loom_block_dev *,
                  loom_partition_scheme_hook, void *);
  const char *name;
  void *data;
  loom_list node;
} loom_partition_scheme;

extern loom_list export_var (loom_partition_schemes);

void export (loomPartitionSchemeRegister) (loom_partition_scheme *);
void export (loomPartitionSchemeUnregister) (loom_partition_scheme *);

#endif