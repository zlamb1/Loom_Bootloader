#ifndef LOOM_FILE_H
#define LOOM_FILE_H 1

#include "loom/compiler.h"
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

int export (loomFileOpen) (struct loom_fs *fs, loom_file *file,
                           const char *path);

int export (loomFileClose) (loom_file *file);

int export (loomFileRead) (loom_file *file, usize nbytes, void *buf,
                           usize *nread);

#endif