#include "loom/dir.h"
#include "loom/assert.h"
#include "loom/fs.h"
#include "loom/print.h"

int
loomDirOpen (loom_fs *fs, loom_dir *dir, const char *path)
{
  loomAssert (fs != null);
  loomAssert (fs->open_dir != null);
  loomAssert (dir != null);
  loomAssert (path != null);

  return fs->open_dir (fs, dir, path);
}

int
loomDirClose (loom_dir *dir)
{
  loomAssert (dir != null);

  auto fs = dir->fs;

  loomAssert (fs->close_dir != null);

  return fs->close_dir (dir);
}

loom_dir_entry *
loomDirRead (loom_dir *dir)
{
  loomAssert (dir != null);

  auto fs = dir->fs;

  loomAssert (fs != null);
  loomAssert (fs->read_dir != null);

  return fs->read_dir (dir);
}