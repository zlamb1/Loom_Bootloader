#include "loom/fs.h"

loom_list loom_fs_list = LOOM_LIST_HEAD (loom_fs_list);
loom_list loom_fs_types = LOOM_LIST_HEAD (loom_fs_types);

loom_fs *loom_prefix_fs = null;