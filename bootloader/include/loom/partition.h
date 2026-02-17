#ifndef LOOM_PARTITION_SCHEME_H
#define LOOM_PARTITION_SCHEME_H 1

#include "loom/compiler.h"
#include "loom/disk.h"
#include "loom/error.h"
#include "loom/types.h"

typedef struct loom_partition_scheme_t
{
  loom_error_t (*iterate) (loom_disk_t *);
  void *data;
  loom_list_t node;
} loom_partition_scheme_t;

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

void LOOM_EXPORT (loom_partition_scheme_register) (loom_partition_scheme_t *);

loom_error_t LOOM_EXPORT (loom_partition_read) (loom_partition_t *p,
                                                loom_usize_t offset,
                                                loom_usize_t count, char *buf);

#endif