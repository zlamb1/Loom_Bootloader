#ifndef LOOM_FS_H
#define LOOM_FS_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

struct loom_block_dev;
struct loom_file;
struct loom_fs;
struct loom_fs_type;

typedef loom_error (*loom_fs_open) (struct loom_fs *fs, struct loom_file *file,
                                    const char *path);

typedef loom_error (*loom_fs_close) (struct loom_file *file);

typedef isize (*loom_fs_read) (struct loom_file *file, usize nbytes,
                               void *buf);

typedef loom_error (*loom_fs_get_uuid) (struct loom_fs *fs, char **uuid);

typedef struct loom_fs
{
  struct loom_block_dev *parent;
  struct loom_fs_type *fs_type;

  loom_fs_open open;
  loom_fs_close close;
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

extern loom_list export_var (loom_fs_list);
extern loom_list export_var (loom_fs_types);

#define loomFsRegister(fs) loomListAddByField (&loom_fs_list, fs, node)
#define loomFsTypeRegister(fs_type)                                           \
  loomListAddByField (&loom_fs_types, fs_type, node)

#define loomFsUnregister(fs)          loomListRemoveByField (fs, node)
#define loomFsTypeUnregister(fs_type) loomListRemoveByField (fs_type, node)

#endif