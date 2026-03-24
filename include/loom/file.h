#include "loom/assert.h"
#ifndef LOOM_FILE_H
#define LOOM_FILE_H 1

#include "loom/error.h"
#include "loom/types.h"

struct loom_fs;

typedef struct loom_file
{
  char *name;

  usize size;
  usize position;

  struct loom_fs *fs;
  void *data;
} loom_file;

loom_error loomFileOpen (struct loom_fs *fs, loom_file *file,
                         const char *path);

isize loomFileRead (loom_file *file, usize nbytes, void *buf);

loom_error loomFileClose (loom_file *file);

#endif