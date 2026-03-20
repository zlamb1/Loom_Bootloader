#ifndef LOOM_OSDEP_H
#define LOOM_OSDEP_H 1

#define LOOM_NOSTDINT
#include "loom/error.h"
#include "loom/types.h"

#define LOOM_O_RDONLY (1 << 0)
#define LOOM_O_WRONLY (1 << 1)
#define LOOM_O_RDWR   (1 << 2)
#define LOOM_O_APPEND (1 << 3)
#define LOOM_O_CREAT  (1 << 4)
#define LOOM_O_TRUNC  (1 << 5)

typedef int loom_file;

typedef struct
{
  usize size;
} loom_file_meta;

typedef struct
{
  usize size;
  void *buf;
} loom_slice_t;

loom_error loomFileOpen (const char *path, uint oflags, loom_file *file);
loom_error loomFileGetMeta (loom_file file, loom_file_meta *meta);
loom_error loomFileSync (loom_file file);
loom_error loomFileClose (loom_file file);

loom_error loomFileRead (loom_file file, void *buf, usize nbytes);
loom_error loomFileReadAt (loom_file file, void *buf, usize offset,
                           usize nbytes);
loom_error loomFileReadAll (loom_file file, loom_slice_t *slice);

loom_error loomFileWrite (loom_file file, void *buf, usize nbytes);
loom_error loomFileWriteAt (loom_file file, void *buf, usize offset,
                            usize nbytes);

const char *loomOsError ();

#endif