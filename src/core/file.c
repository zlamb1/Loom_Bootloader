#include "loom/file.h"
#include "loom/assert.h"
#include "loom/fs.h"

int
loomFileOpen (struct loom_fs *fs, loom_file *file, const char *path)
{
  loomAssert (fs != null);
  loomAssert (fs->open != null);
  loomAssert (file != null);

  return fs->open (fs, file, path);
}

int
loomFileClose (loom_file *file)
{
  loomAssert (file != null);
  loomAssert (file->fs != null);

  auto fs = file->fs;

  loomAssert (fs->close != null);

  return fs->close (file);
}

int
loomFileRead (loom_file *file, usize nbytes, void *buf, usize *nread)
{
  loomAssert (file != null);
  loomAssert (file->fs != null);

  auto fs = file->fs;

  loomAssert (fs->read != null);

  loomAssert (!nbytes || buf != null);

  return fs->read (file, nbytes, buf, nread);
}
