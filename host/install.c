#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "osdep.h"

#include "loom/block_dev.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"

static void
errorWith (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "\033[1;31merror: \033[0m");
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
  exit (1);
}

static void
validateImage (loom_slice_t img_slice)
{
  if (img_slice.size < 512)
    errorWith ("invalid image: too small");

  for (int i = 446; i < 510; ++i)
    if (((byte *) img_slice.buf)[i])
      errorWith (
          "invalid image: non-zero bytes collide with MBR partition table");
}

static loom_error
fileBlockDevRead (loom_block_dev *block_dev, usize block, usize count,
                  char *buf)
{
  loom_file file = *(loom_file *) block_dev->data;
  usize offset = block * block_dev->block_size;
  usize nbytes = count * block_dev->block_size;

  if (loomFileReadAt (file, buf, offset, nbytes))
    return loomErrorFmt (LOOM_ERR_PLATFORM, "%s", loomOsError ());

  return LOOM_ERR_NONE;
}

typedef struct
{
  usize count;
  usize gap;
} partition_hook_ctx;

static int
mbrPartitionHook (loom_block_dev *block_dev, loom_partition *partition,
                  void *p)
{
  partition_hook_ctx *ctx = p;

  (void) block_dev;

  if (!ctx->count || ctx->gap > partition->offset)
    ctx->gap = partition->offset;

  ctx->count += 1;

  return 0;
}

void
embedImage (loom_slice_t slice, const char *out_path, loom_file out_img)
{
  extern loom_partition_scheme mbr_partition_scheme;

  loom_file_meta file_meta;
  loom_block_dev file_block_dev;
  loom_block_dev_init_t init = { 0 };
  partition_hook_ctx ctx = { 0 };

  if (loomFileGetMeta (out_img, &file_meta))
    errorWith ("failed to get out image file meta: %s", loomOsError ());

  init.block_size = 512;
  init.blocks = file_meta.size / 512;
  init.data = &out_img;
  init.read = fileBlockDevRead;

  if (file_meta.size % 512)
    init.blocks += 1;

  loomBlockDevInit (&file_block_dev, &init);

  if (mbr_partition_scheme.iterate (&mbr_partition_scheme, &file_block_dev,
                                    mbrPartitionHook, &ctx))
    errorWith ("image '%s' does not contain MBR: %s", out_path,
               loomErrorGet ());

  if (!ctx.count)
    fprintf (stderr, "\033[1;93mwarning: \033[0mMBR partition table is empty; "
                     "embedding anyways\n");

  if (ctx.count)
    {
      if (slice.size > ctx.gap * 512)
        errorWith ("gap of %lu bytes is too small to embed core",
                   ctx.gap * 512);
    }
  else if (slice.size > file_meta.size)
    errorWith ("disk size of %lu bytes is too small to contain core",
               file_meta.size);

  if (loomFileWriteAt (out_img, (char *) slice.buf, 0, 446)
      || loomFileWriteAt (out_img, (char *) slice.buf + 512, 512,
                          slice.size - 512))
    errorWith ("failed to write '%s': %s", out_path, loomOsError ());
}

int
main (int argc, char *argv[])
{
  const char *loom_path;
  const char *out_path;

  loom_file loom_img, out_img;
  loom_slice_t slice;

  if (argc != 3)
    errorWith ("expected %s [LOOM PATH] [OUT PATH]",
               argc ? argv[0] : "loom-install");

  loom_path = argv[1];
  out_path = argv[2];

  if (loomFileOpen (loom_path, LOOM_O_RDONLY, &loom_img))
    errorWith ("failed to open '%s': %s", loom_path, loomOsError ());

  if (loomFileOpen (out_path, LOOM_O_RDWR, &out_img))
    errorWith ("failed to open '%s': %s", out_path, loomOsError ());

  if (loomFileReadAll (loom_img, &slice))
    errorWith ("failed to read '%s': %s", loom_path, loomOsError ());

  validateImage (slice);

  embedImage (slice, out_path, out_img);

  if (loomFileSync (out_img))
    errorWith ("failed to sync '%s': %s", out_path, loomOsError ());

  return 0;
}