#include "loom/fs.h"
#include "loom/assert.h"
#include "loom/mm.h"

loom_list loom_fs_list = LOOM_LIST_HEAD (loom_fs_list);
loom_list loom_fs_types = LOOM_LIST_HEAD (loom_fs_types);

loom_fs *loom_prefix_fs = null;

void
loomFsFree (loom_fs *fs)
{
  loomAssert (fs != null);

  if (fs->free != null)
    fs->free (fs);

  loomFree (fs);
}