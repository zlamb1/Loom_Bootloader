#ifndef LOOM_FS_H
#define LOOM_FS_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

struct loom_block_dev_t;
struct loom_fs_t;

typedef struct
{
  loom_usize_t size;
  void *data;
} loom_file_t;

typedef struct
{
  loom_usize_t size;
  char *data;
} loom_fs_uuid_t;

typedef int (*loom_fs_read) (struct loom_fs_t *fs, const char *path,
                             loom_file_t *file);

typedef int (*loom_fs_get_uuid) (struct loom_fs_t *fs, loom_fs_uuid_t *uuid);

typedef struct loom_fs_t
{
  struct loom_block_dev_t *parent;
  loom_fs_read read;
  loom_fs_get_uuid get_uuid;
  void *data;
  loom_list_t node;
} loom_fs_t;

typedef struct
{
  loom_fs_t *(*probe) (struct loom_block_dev_t *);
  void *data;
  loom_list_t node;
} loom_fs_type_t;

extern loom_list_t LOOM_EXPORT_VAR (loom_fs_list);
extern loom_list_t LOOM_EXPORT_VAR (loom_fs_types);

#define loom_fs_register(fs) loom_list_add_by_field (&loom_fs_list, fs, node)
#define loom_fs_type_register(fs_type)                                        \
  loom_list_add_by_field (&loom_fs_types, fs_type, node)

#define loom_fs_unregister(fs) loom_list_remove_by_field (fs, node)
#define loom_fs_type_unregister(fs_type)                                      \
  loom_list_remove_by_field (fs_type, node)

#endif