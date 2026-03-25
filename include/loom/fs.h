#ifndef LOOM_FS_H
#define LOOM_FS_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

struct loom_block_dev;
struct loom_file;
struct loom_dir;
struct loom_dir_entry;
struct loom_fs;
struct loom_fs_type;

typedef int (*loom_fs_open) (struct loom_fs *fs, struct loom_file *file,
                             const char *path);

typedef int (*loom_fs_open_dir) (struct loom_fs *fs, struct loom_dir *dir,
                                 const char *path);

typedef int (*loom_fs_close) (struct loom_file *file);

typedef int (*loom_fs_close_dir) (struct loom_dir *dir);

typedef int (*loom_fs_read) (struct loom_file *file, usize nbytes, void *buf,
                             usize *nread);

typedef struct loom_dir_entry *(*loom_fs_read_dir) (struct loom_dir *dir);

typedef int (*loom_fs_get_uuid) (struct loom_fs *fs, char **uuid);

typedef void (*loom_fs_free) (struct loom_fs *fs);

typedef struct loom_fs
{
  struct loom_block_dev *parent;
  struct loom_fs_type *fs_type;

  loom_fs_open open;
  loom_fs_open_dir open_dir;
  loom_fs_close close;
  loom_fs_close_dir close_dir;
  loom_fs_read read;
  loom_fs_read_dir read_dir;
  loom_fs_get_uuid get_uuid;
  loom_fs_free free;

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

extern loom_fs *loom_prefix_fs;

#define loomFsRegister(fs) loomListAddByField (&loom_fs_list, fs, node)
#define loomFsTypeRegister(fs_type)                                           \
  loomListAddByField (&loom_fs_types, fs_type, node)

#define loomFsUnregister(fs)          loomListRemoveByField (fs, node)
#define loomFsTypeUnregister(fs_type) loomListRemoveByField (fs_type, node)

void loomFsFree (loom_fs *fs);

#endif