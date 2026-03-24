#ifndef LOOM_FILE_H
#define LOOM_FILE_H 1

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

int loomFileOpen (struct loom_fs *fs, loom_file *file, const char *path);

int loomFileClose (loom_file *file);

int loomFileRead (loom_file *file, usize nbytes, void *buf, usize *nread);

#endif