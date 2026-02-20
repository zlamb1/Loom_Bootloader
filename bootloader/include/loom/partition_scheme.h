#ifndef LOOM_PARTITION_SCHEME_H
#define LOOM_PARTITION_SCHEME_H 1

#include "loom/compiler.h"
#include "loom/disk.h"
#include "loom/error.h"
#include "loom/list.h"

typedef struct loom_partition_scheme_t
{
  loom_error_t (*iterate) (loom_disk_t *);
  void *data;
  loom_list_t node;
} loom_partition_scheme_t;

extern loom_list_t LOOM_EXPORT_VAR (loom_partition_schemes);

void LOOM_EXPORT (loom_partition_scheme_register) (loom_partition_scheme_t *);
void
    LOOM_EXPORT (loom_partition_scheme_unregister) (loom_partition_scheme_t *);

#endif