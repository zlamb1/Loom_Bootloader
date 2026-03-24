#ifndef LOOM_FILE_H
#define LOOM_FILE_H 1

#include "loom/types.h"

struct loom_fs;

typedef struct
{
  char *name;

  usize size;
  usize offset;

  struct loom_fs *fs;
  void *data;
} loom_file;

#endif