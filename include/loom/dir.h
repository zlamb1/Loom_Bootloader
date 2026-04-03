#ifndef LOOM_DIR_H
#define LOOM_DIR_H 1

#include "loom/compiler.h"
#include "loom/types.h"

struct loom_fs;

typedef struct loom_dir
{
  char *name;

  struct loom_fs *fs;

  void *data;
} loom_dir;

typedef struct loom_dir_entry
{
  char *name;

  bool is_file : 1;
  bool is_dir : 1;
} loom_dir_entry;

int export (loomDirOpen) (struct loom_fs *fs, loom_dir *dir, const char *path);

int export (loomDirClose) (loom_dir *dir);

loom_dir_entry *export (loomDirRead) (loom_dir *dir);

#endif