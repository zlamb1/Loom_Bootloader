#ifndef LOOM_FS_H
#define LOOM_FS_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

struct loom_block_dev;
struct loom_fs;
struct loom_fs_type;

typedef struct
{
  usize size;
  void *data;
} loom_file;

typedef struct
{
  usize size;
  char *data;
} loom_fs_uuid;

typedef int (*loom_fs_read) (struct loom_fs *fs, const char *path,
                             loom_file *file);

typedef int (*loom_fs_get_uuid) (struct loom_fs *fs, loom_fs_uuid *uuid);

typedef struct loom_fs
{
  struct loom_block_dev *parent;
  struct loom_fs_type *fs_type;
  loom_fs_read read;
  loom_fs_get_uuid get_uuid;
  void *data;
  loom_list node;
} loom_fs;

typedef struct loom_fs_type
{
  const char *name;
  loom_fs *(*probe) (struct loom_block_dev *);
  void *data;
  loom_list node;
} loom_fs_type;

extern loom_list LOOM_EXPORT_VAR (loom_fs_list);
extern loom_list LOOM_EXPORT_VAR (loom_fs_types);

#define loom_fs_register(fs) loom_list_add_by_field (&loom_fs_list, fs, node)
#define loom_fs_type_register(fs_type)                                        \
  loom_list_add_by_field (&loom_fs_types, fs_type, node)

#define loom_fs_unregister(fs) loom_list_remove_by_field (fs, node)
#define loom_fs_type_unregister(fs_type)                                      \
  loom_list_remove_by_field (fs_type, node)

#endif