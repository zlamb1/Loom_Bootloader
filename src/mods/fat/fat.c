#include "loom/assert.h"
#include "loom/block_dev.h"
#include "loom/dir.h"
#include "loom/error.h"
#include "loom/file.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/string.h"

#include "fat.h"

static inline loom_fs *fatFsProbe (loom_block_dev *block_dev);

loom_fs_type fat_fs_type = {
  .name = "fat",
  .probe = fatFsProbe,
};

LOOM_MOD (fat)

static inline loom_fs *
fatFsProbe (loom_block_dev *block_dev)
{
  fat_bpb bpb;
  fat_fs *fs = loomAlloc (sizeof (*fs));

  if (fs == null)
    return null;

  if (loomBlockDevRead (block_dev, 0, sizeof (bpb), (char *) &bpb))
    goto out;

  auto signature = loomEndianLoad (bpb.signature);
  if (signature != FAT_BOOT_SIGNATURE)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad boot signature 0x%.4lx",
                    (ulong) signature);
      goto out;
    }

  auto bytes_per_sect = loomEndianLoad (bpb.bytes_per_sect);
  if (bytes_per_sect != 512 && bytes_per_sect != 1024 && bytes_per_sect != 2048
      && bytes_per_sect != 4096)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad bytes per sector %lu",
                    (ulong) bytes_per_sect);
      goto out;
    }

  auto sects_per_cluster = bpb.sects_per_cluster;
  if (sects_per_cluster == 0
      || (sects_per_cluster & (sects_per_cluster - 1)) != 0)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad sectors per cluster %lu",
                    (ulong) sects_per_cluster);
      goto out;
    }

  u32 bytes_per_cluster = (u32) bytes_per_sect * (u32) sects_per_cluster;
  if (bytes_per_cluster > 32 * 1024)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "bytes per cluster must not exceed 32KiB");
      goto out;
    }

  auto reserved_sects = loomEndianLoad (bpb.reserved_sects);
  if (reserved_sects == 0)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad number of reserved sectors %lu",
                    (ulong) reserved_sects);
      goto out;
    }

  auto fat_count = bpb.fats;
  if (!fat_count)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad number of FATs %lu",
                    (ulong) fat_count);
      goto out;
    }

  auto root_entry_count = loomEndianLoad (bpb.root_entry_count);
  auto fatsz16 = loomEndianLoad (bpb.fatsz16);
  auto sects16 = loomEndianLoad (bpb.sects16);

  u32 root_dir_sects = (((u32) root_entry_count * 32) + (bytes_per_sect - 1))
                       / bytes_per_sect;
  u32 fat_sects;
  u32 sects;

  if (fatsz16)
    fat_sects = fatsz16;
  else
    fat_sects = loomEndianLoad (bpb.ebpb32.fatsz32);

  if (!fat_sects)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad FAT size %lu", (ulong) fat_sects);
      goto out;
    }

  if (sects16)
    sects = sects16;
  else
    sects = loomEndianLoad (bpb.sects32);

  if (!sects)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad number of sectors %lu",
                    (ulong) sects);
      goto out;
    }

  u32 meta_sects;
  if (loomMul (fat_count, fat_sects, &meta_sects)
      || loomAdd (meta_sects, reserved_sects, &meta_sects)
      || loomAdd (meta_sects, root_dir_sects, &meta_sects))
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad number of metadata sectors");
      goto out;
    }

  if (meta_sects > sects || sects - meta_sects < 2)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "too many metadata sectors %lu",
                    (ulong) meta_sects);
      goto out;
    }

  auto data_sects = sects - meta_sects;
  auto cluster_count = data_sects / sects_per_cluster;
  u8 fat_type;

  if (!cluster_count)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad cluster count %lu",
                    (ulong) cluster_count);
      goto out;
    }

  if (cluster_count < 4085)
    fat_type = FAT_TYPE_12;
  else if (cluster_count < 65525)
    fat_type = FAT_TYPE_16;
  else
    fat_type = FAT_TYPE_32;

  // Now we do type specific checking of fields.
  if (fat_type != FAT_TYPE_32 && reserved_sects != 1)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT12/16 mandates reserved sector count of 1; got %lu",
                    (ulong) reserved_sects);
      goto out;
    }

  if (fat_type != FAT_TYPE_32
      && ((root_entry_count * 32) & (bytes_per_sect - 1)))
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT12/16 mandates root directory count x 32 should be a "
                    "multiple of %lu; got %lu",
                    (ulong) bytes_per_sect, (ulong) root_entry_count * 32);
      goto out;
    }

  if (fat_type != FAT_TYPE_32 && !root_entry_count)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT12/16 mandates non-zero root entry count; got 0");
      goto out;
    }

  if (fat_type == FAT_TYPE_32 && root_entry_count)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT32 mandates root entry count is zero; got %lu",
                    (ulong) root_entry_count);
      goto out;
    }

  if (fat_type == FAT_TYPE_32 && sects16)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT32 mandates 16-bit sector count is zero; got %lu",
                    (ulong) sects16);
      goto out;
    }

  if (fat_type == FAT_TYPE_32 && fatsz16)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS,
                    "FAT32 mandates 16-bit FAT size is zero; got %lu",
                    (ulong) fatsz16);
      goto out;
    }

  fs->super.open = fatOpen;
  fs->super.open_dir = fatOpenDir;
  fs->super.close = fatClose;
  fs->super.close_dir = fatCloseDir;
  fs->super.read = fatRead;
  fs->super.read_dir = fatReadDir;
  fs->super.free = fatFree;
  fs->super.data = fs;

  fs->type = fat_type;
  fs->reserved_sects = reserved_sects;
  fs->bytes_per_sect = bytes_per_sect;
  fs->sects_per_cluster = sects_per_cluster;
  fs->fat_count = fat_count;
  fs->fat_sects = fat_sects;
  fs->data_off = meta_sects * bytes_per_sect;

  if (fat_type == FAT_TYPE_32)
    fs->root_cluster = loomEndianLoad (bpb.ebpb32.root_cluster);
  else
    fs->root_entry_count = root_entry_count;

  fs->scratch.sect = 0;
  auto size = bytes_per_cluster;

  if (sects_per_cluster == 1)
    size *= 2;

  if ((fs->scratch.buf = loomAlloc (size)) == null)
    goto out;

  return &fs->super;

out:
  loomFree (fs);
  return null;
}

static int
fatSearchPath (fat_iterator_ctx *ctx, bool *found, bool *oroot,
               const char *path)
{
  usize index = 0;
  bool root = true;

  for (;;)
    {
      char ch = path[index];
      usize run = 0;

      while (ch == '/')
        {
          index += 1;
          ch = path[index];
        }

      while (ch != '\0' && ch != '/')
        {
          run += 1;
          ch = path[index + run];
        }

      if (run)
        {
          root = false;

          if (fatFindDirEntry (run, path + index, ctx))
            return -1;

          if (ch != '\0' && fatIteratorReset (ctx, &ctx->entry))
            return -1;

          *found = true;
        }

      index += run;

      if (ch == '\0')
        break;
    }

  if (oroot != null)
    {
      *oroot = root;
      if (root)
        *found = true;
    }

  return 0;
}

int
fatOpen (loom_fs *super, loom_file *file, const char *path)
{
  fat_iterator_ctx ctx;
  fat_fs *fs;

  bool found = false;

  loomAssert (super != null);
  loomAssert (path != null);
  loomAssert (file != null);

  fs = (fat_fs *) super->data;

  file->data = null;

  if (fatIteratorInit (&ctx, null, fs))
    return -1;

  if (fatSearchPath (&ctx, &found, null, path))
    return -1;

  if (!found || !fatIsFile (&ctx.entry))
    {
      loomError (LOOM_ERR_ISDIR);
      return -1;
    }

  auto entry = ctx.entry;

  fat_file_ctx *file_ctx = loomAlloc (sizeof (*file_ctx));

  if (file_ctx == null)
    return -1;

  file->size = loomEndianLoad (entry.file_size);
  file->position = 0;
  file->fs = super;
  file->data = file_ctx;

  file_ctx->cluster = fatDirEntryGetCluster (&entry);
  file_ctx->pos = file_ctx->cluster;

  return 0;
}

int
fatOpenDir (loom_fs *super, loom_dir *dir, const char *path)
{
  fat_iterator_ctx ctx;
  fat_fs *fs;

  bool found, root;

  loomAssert (super != null);
  loomAssert (path != null);
  loomAssert (dir != null);

  fs = (fat_fs *) super->data;

  dir->name = null;
  dir->fs = null;
  dir->data = null;

  if (fatIteratorInit (&ctx, null, fs))
    return -1;

  if (fatSearchPath (&ctx, &found, &root, path))
    return -1;

  if (!found || (!root && !fatIsDirectory (&ctx.entry)))
    {
      loomError (LOOM_ERR_NOTDIR);
      return -1;
    }

  fat_dir_ctx *dir_ctx = loomAlloc (sizeof (*dir_ctx));

  if (dir_ctx == null)
    {
      loomFree (dir->name);
      dir->name = null;
      return -1;
    }

  loomMemSet (dir_ctx, 0, sizeof (*dir_ctx));

  if (!root)
    {
      auto entry = ctx.entry;

      if (fatAllocSFN (&dir->name, &entry))
        {
          loomFree (dir_ctx);
          return -1;
        }

      if (fatIteratorInit (&dir_ctx->iterator_ctx, &entry, fs))
        {
          loomFree (dir->name);
          loomFree (dir_ctx);
          dir->name = null;
          return -1;
        }
    }
  else
    {
      if (fatIteratorInit (&dir_ctx->iterator_ctx, null, fs))
        {
          loomFree (dir_ctx);
          return -1;
        }

      if ((dir->name = loomAlloc (2)) == null)
        {
          loomFree (dir_ctx);
          return -1;
        }

      dir->name[0] = '/';
      dir->name[1] = '\0';
    }

  dir->fs = super;
  dir->data = dir_ctx;

  return 0;
}

int
fatClose (loom_file *file)
{
  loomAssert (file != null);
  loomAssert (file->data != null);

  loomFree (file->data);

  file->data = null;

  return 0;
}

int
fatCloseDir (loom_dir *dir)
{
  loomAssert (dir != null);
  loomAssert (dir->data != null);

  auto ctx = (fat_dir_ctx *) dir->data;

  loomFree (dir->name);
  loomFree (ctx->d_entry.name);
  loomFree (ctx);

  dir->name = dir->data = null;

  return 0;
}

int
fatRead (loom_file *file, usize nbytes, void *buf, usize *nread)
{
  loomAssert (file != null);
  loomAssert (file->data != null);

  auto super = file->fs;

  loomAssert (super != null);
  loomAssert (super->data != null);

  loom_error error;

  fat_fs *fs = super->data;

  fat_file_ctx *file_ctx = file->data;
  auto file_size = file->size;

  auto cluster_limit = fatGetClusterLimit (fs->type);
  auto cluster_size = fatGetClusterSize (fs);

  auto cluster_buf = loomAlloc (fs->bytes_per_sect * 2);

  if (cluster_buf == null)
    {
      loomError (LOOM_ERR_ALLOC);
      return -1;
    }

  for (;;)
    {
      auto position = file->position;
      auto cluster = file_ctx->pos;
      auto cluster_off = position % cluster_size;

      usize read = 0;

      if (!nbytes || position >= file_size || cluster >= cluster_limit)
        break;

      auto max_read = file_size - position;
      if (max_read > nbytes)
        max_read = nbytes;

      auto offset = fatGetClusterOffset (cluster, fs) + cluster_off;

      while (read < max_read)
        {
          u32 old_cluster = cluster;
          bool advance = false;

          auto to_read = max_read - read;

          if (to_read >= cluster_size - cluster_off)
            {
              advance = true;
              to_read = cluster_size - cluster_off;

              error = fatNextCluster (cluster, &cluster, cluster_buf, fs);
              if (error != LOOM_ERR_NONE)
                goto out;
            }

          read += to_read;
          cluster_off = 0;

          // Note: Overflow is guarded here by the cluster limit
          // precondition.
          if (advance && old_cluster + 1 != cluster)
            break;
        }

      if (read > max_read)
        read = max_read;

      if ((error = loomBlockDevRead (super->parent, offset, read, buf)))
        goto out;

      nbytes -= read;
      file->position += read;
      buf = (char *) buf + read;

      if (nread != null)
        *nread += read;

      file_ctx->pos = cluster;
    }

  loomFree (cluster_buf);
  return 0;

out:
  loomFree (cluster_buf);
  loomError (error);
  return -1;
}

loom_dir_entry *
fatReadDir (loom_dir *dir)
{
  fat_dir_ctx *ctx;

  loomAssert (dir != null);
  loomAssert (dir->data != null);

  ctx = dir->data;

  loomErrorClear ();

  auto entry = &ctx->iterator_ctx.entry;
  auto d_entry = &ctx->d_entry;

  loomFree (d_entry->name);
  d_entry->name = null;

  for (;;)
    {
      if (fatIterateDirEntries (&ctx->iterator_ctx)
          || !ctx->iterator_ctx.has_next)
        return null;

      if ((entry->attribs & FAT_FILE_ATTR_VOLUME_ID)
          || (entry->attribs & FAT_FILE_ATTR_LFN_MASK) == FAT_FILE_ATTR_LFN)
        continue;

      if (fatAllocSFN (&d_entry->name, entry))
        return null;

      d_entry->is_dir = !!fatIsDirectory (entry);
      d_entry->is_file = !!fatIsFile (entry);

      break;
    }

  return d_entry;
}

void
fatFree (loom_fs *super)
{
  loomAssert (super != null);
  loomAssert (super->data != null);

  fat_fs *fs = super->data;
  loomFree (fs->scratch.buf);
}

LOOM_MOD_INIT () { loomFsTypeRegister (&fat_fs_type); }

LOOM_MOD_DEINIT () { loomFsTypeUnregister (&fat_fs_type); }
