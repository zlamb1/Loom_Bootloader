#ifndef LOOM_PARTITION_H
#define LOOM_PARTITION_H 1

#include "loom/partition_scheme.h"

typedef struct loom_partition_t
{
  loom_bool_t root;
  loom_usize_t start;
  loom_usize_t length;
  loom_partition_scheme_t *partition_scheme;

  union
  {
    loom_disk_t *disk;
    struct loom_partition_t *parent;
  };
} loom_partition_t;

extern loom_list_t loom_partition_schemes;

loom_error_t LOOM_EXPORT (loom_partition_read) (loom_partition_t *p,
                                                loom_usize_t offset,
                                                loom_usize_t count, char *buf);

#endif