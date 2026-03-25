#include "loom/block_dev.h"
#include "loom/error.h"
#include "loom/string.h"

#include "fat.h"

static inline uchar force_inline
fatCharConv (uchar ch)
{
  if (ch >= 'a' && ch <= 'z')
    ch -= 0x20;
  return ch;
}

static inline bool
fatFilenameCmp (usize size, const char *file_name, fat_dir_entry *dir_entry)
{
  uint base_size = 8, ext_size = 3;
  auto fname = (const uchar *) file_name;
  auto dfname = (const uchar *) dir_entry->file_name;

  if (!size || size > 12 || dfname[0] == FAT_DIR_ENTRY_END
      || dfname[0] == FAT_DIR_ENTRY_FREE || dfname[0] == 0x20)
    return false;

  for (int i = 7; i > 0; --i, --base_size)
    if (dfname[i] != 0x20)
      break;

  for (int i = 2; i >= 0; --i, --ext_size)
    if (dfname[i + 8] != 0x20)
      break;

  if (size < base_size)
    return false;

  for (uint i = 0; i < base_size; ++i, --size, ++fname)
    {
      auto ch = dfname[i];
      if (!i && ch == 0x05)
        ch = 0xE5;
      if (fatCharConv (fname[0]) != fatCharConv (ch))
        return false;
    }

  bool ext = false;
  if (size && fname[0] == '.')
    {
      --size, ++fname;
      ext = true;
    }

  if (ext_size)
    {
      if (!ext || ext_size != size)
        return false;
      for (uint i = 0; i < ext_size; ++i, ++fname)
        if (fatCharConv (fname[0]) != fatCharConv (dfname[i + 8]))
          return false;
      return true;
    }

  return !size;
}

int
fatIteratorInit (fat_iterator_ctx *ctx, fat_dir_entry *dir, fat_fs *fs)
{
  ctx->fs = fs;
  return fatIteratorReset (ctx, dir);
}

int
fatIteratorReset (fat_iterator_ctx *ctx, fat_dir_entry *dir)
{
  loom_error error;
  auto fs = ctx->fs;

  if (dir != null && !fatIsDirectory (dir))
    {
      loomError (LOOM_ERR_NOTDIR);
      return -1;
    }

  if (fs->type == FAT_TYPE_32 || dir != null)
    {
      if (dir == null)
        ctx->cluster = fs->root_cluster;
      else
        ctx->cluster = fatDirEntryGetCluster (dir);
      if ((error = fatReadCluster (ctx->cluster, fs->scratch.buf, fs)))
        {
          loomError (error);
          return -1;
        }
    }
  else
    {
      ctx->r_entry = 0;
      ctx->count = fs->root_entry_count;

      if ((error
           = loomBlockDevRead (fs->super.parent, fatGetRootEntriesOffset (fs),
                               fs->bytes_per_sect * 2, fs->scratch.buf)))
        {
          loomError (error);
          return -1;
        }
    }

  ctx->root16_12 = (fs->type != FAT_TYPE_32 && dir == null);
  ctx->offset = 0;

  return 0;
}

static inline bool
fatHandleDirEntry (fat_iterator_ctx *ctx, void *buf)
{
  loomMemCopy (&ctx->entry, (char *) buf + ctx->offset,
               sizeof (fat_dir_entry));

  auto file_name = (uchar *) ctx->entry.file_name;

  if (file_name[0] == FAT_DIR_ENTRY_END)
    {
      ctx->has_next = false;
      return false;
    }

  ctx->offset += sizeof (fat_dir_entry);
  ctx->has_next = true;

  if (file_name[0] == FAT_DIR_ENTRY_FREE
      || (ctx->entry.attribs & FAT_FILE_ATTR_LFN_MASK) == FAT_FILE_ATTR_LFN)
    return true;

  return false;
}

int
fatIterateDirEntriesRoot16_12 (fat_iterator_ctx *ctx)
{
  loom_error error;
  auto fs = ctx->fs;

  for (;;)
    {
      if (ctx->r_entry >= ctx->count)
        {
          ctx->has_next = false;
          return LOOM_ERR_NONE;
        }

      if (ctx->offset >= fs->bytes_per_sect * 2)
        {
          auto read = (usize) fs->bytes_per_sect * 2;
          auto offset = fatGetRootEntriesOffset (fs)
                        + (ctx->r_entry * 32 / read) * read;

          if ((error = loomBlockDevRead (fs->super.parent, offset, read,
                                         fs->scratch.buf)))
            {
              loomError (error);
              return -1;
            }

          ctx->offset = 0;
        }

      bool cont = fatHandleDirEntry (ctx, fs->scratch.buf);

      if (ctx->has_next)
        ctx->r_entry += 1;

      if (!cont)
        break;
    }

  return 0;
}

int
fatIterateDirEntriesCluster (fat_iterator_ctx *ctx)
{
  loom_error error;
  u32 cluster_limit;

  auto fs = ctx->fs;
  auto buf = fs->scratch.buf;
  auto bytes_per_cluster
      = (usize) fs->bytes_per_sect * (usize) fs->sects_per_cluster;

  cluster_limit = fatGetClusterLimit (fs->type);

  for (;;)
    {
      if (ctx->offset >= bytes_per_cluster)
        {
          if ((error = fatNextCluster (ctx->cluster, &ctx->cluster, buf, fs)))
            {
              loomError (error);
              return -1;
            }

          if (ctx->cluster >= cluster_limit)
            {
              ctx->has_next = false;
              return 0;
            }

          if ((error = fatReadCluster (ctx->cluster, buf, fs)))
            {
              loomError (error);
              return -1;
            }

          ctx->offset = 0;
        }

      if (!fatHandleDirEntry (ctx, buf))
        break;
    }

  return 0;
}

int
fatIterateDirEntries (fat_iterator_ctx *ctx)
{
  if (ctx->root16_12)
    return fatIterateDirEntriesRoot16_12 (ctx);
  else
    return fatIterateDirEntriesCluster (ctx);
}

int
fatFindDirEntry (usize size, const char *name, fat_iterator_ctx *ctx)
{
  for (;;)
    {
      if (fatIterateDirEntries (ctx))
        return -1;

      if (!ctx->has_next)
        {
          loomErrorFmt (LOOM_ERR_NOENT, "file not found");
          return -1;
        }

      if (ctx->entry.attribs & FAT_FILE_ATTR_VOLUME_ID)
        continue;

      if (!fatFilenameCmp (size, name, &ctx->entry))
        continue;

      break;
    }

  return 0;
}