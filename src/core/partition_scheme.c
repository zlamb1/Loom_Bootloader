#include "loom/partition_scheme.h"
#include "loom/assert.h"
#include "loom/list.h"

loom_list loom_partition_schemes = LOOM_LIST_HEAD (loom_partition_schemes);

void
loomPartitionSchemeRegister (loom_partition_scheme *partition_scheme)
{
  loomAssert (partition_scheme != NULL);
  loomListAdd (&loom_partition_schemes, &partition_scheme->node);
}

void
loomPartitionSchemeUnregister (loom_partition_scheme *partition_scheme)
{
  loomAssert (partition_scheme != NULL);
  loomListRemove (&partition_scheme->node);
}