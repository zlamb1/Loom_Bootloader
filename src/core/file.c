#include "loom/file.h"
#include "loom/assert.h"
#include "loom/fs.h"

loom_error
loomFileOpen (struct loom_fs *fs, loom_file *file, const char *path)
{
  loomAssert (fs != null);
  loomAssert (fs->open != null);
  loomAssert (file != null);

  return fs->open (fs, file, path);
}

isize
loomFileRead (loom_file *file, usize nbytes, void *buf)
{
  loomAssert (file != null);
  loomAssert (file->fs != null);

  auto fs = file->fs;

  loomAssert (fs->read != null);

  loomAssert (!nbytes || buf != null);

  auto read = fs->read (file, nbytes, buf);

  if (read < 0)
    return read;

  file->position += (usize) read;

  return read;
}

loom_error
loomFileClose (loom_file *file)
{
  loomAssert (file != null);
  loomAssert (file->fs != null);

  auto fs = file->fs;

  loomAssert (fs->close != null);

  return fs->close (file);
}