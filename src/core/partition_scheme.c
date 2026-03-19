#include "loom/partition_scheme.h"
#include "loom/assert.h"
#include "loom/list.h"

loom_list loom_partition_schemes = LOOM_LIST_HEAD (loom_partition_schemes);

void
loom_partition_scheme_register (loom_partition_scheme *partition_scheme)
{
  loom_assert (partition_scheme != NULL);
  loom_list_prepend (&loom_partition_schemes, &partition_scheme->node);
}

void
loom_partition_scheme_unregister (loom_partition_scheme *partition_scheme)
{
  loom_assert (partition_scheme != NULL);
  loom_list_remove (&partition_scheme->node);
}