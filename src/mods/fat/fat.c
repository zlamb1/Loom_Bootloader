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

static int fatRead (loom_fs *_fs, const char *path, loom_file *file);

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

  fs->super.read = fatRead;
  fs->super.data = fs;

  fs->type = fat_type;
  fs->reserved_sects = reserved_sects;
  fs->bytes_per_sect = bytes_per_sect;
  fs->sects_per_cluster = sects_per_cluster;
  fs->fat_count = fat_count;

  if (fat_type == FAT_TYPE_32)
    fs->root_cluster = loomEndianLoad (bpb.ebpb32.root_cluster);
  else
    fs->root_entry_count = root_entry_count;

  fs->fat_sects = fat_sects;

  return &fs->super;

out:
  loomFree (fs);
  return null;
}

static int
fatRead (loom_fs *_fs, const char *path, loom_file *file)
{
  loom_error error;
  fat_iterator_ctx ctx;
  fat_fs *fs;

  loomAssert (_fs != null);
  loomAssert (path != null);
  loomAssert (file != null);

  fs = (fat_fs *) _fs->data;

  file->data = null;

  if ((error = fatIteratorInit (&ctx, null, fs)))
    {
      loomError (error);
      return -1;
    }

  usize index = 0;
  bool found_one = false;

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
          if ((error = fatFindDirEntry (run, path + index, &ctx)))
            goto out;

          if (ch != '\0' && (error = fatIteratorReset (&ctx, &ctx.entry)))
            goto out;

          found_one = true;
        }

      index += run;

      if (ch == '\0')
        break;
    }

  if (!found_one)
    {
      error = LOOM_ERR_ISDIR;
      goto out;
    }

  auto entry = ctx.entry;

  if (!fatIsFile (&entry))
    {
      error = LOOM_ERR_ISDIR;
      goto out;
    }

  usize file_offset = 0;
  usize file_size = loomEndianLoad (entry.file_size);

  auto cluster = fatDirEntryGetCluster (&entry);
  auto cluster_size
      = (usize) fs->bytes_per_sect * (usize) fs->sects_per_cluster;
  auto cluster_limit = fatGetClusterLimit (fs->type);

  file->size = file_size;
  file->data = loomAlloc (file_size);

  if (file->data == null)
    {
      error = LOOM_ERR_ALLOC;
      goto out;
    }

  while (file_size)
    {
      auto read = file_size;

      if (cluster >= cluster_limit)
        break;

      if (read > cluster_size)
        read = cluster_size;

      if (read < cluster_size)
        {
          if ((error = fatReadCluster (cluster, ctx.buf, fs)))
            goto out;
          loomMemCopy ((char *) file->data + file_offset, ctx.buf, read);
        }
      else if ((error = fatReadCluster (
                    cluster, (char *) file->data + file_offset, fs)))
        goto out;

      if (read == cluster_size
          && (error = fatNextCluster (cluster, &cluster, ctx.buf, fs)))
        goto out;

      file_offset += read;
      file_size -= read;
    }

  if (file_size)
    {
      error = LOOM_ERR_BAD_FS;
      goto out;
    }

  return 0;

out:
  loomFree (file->data);
  fatIteratorDeinit (&ctx);
  file->size = 0;
  file->data = null;
  loomError (error);
  return -1;
}

LOOM_MOD_INIT () { loomFsTypeRegister (&fat_fs_type); }

LOOM_MOD_DEINIT () { loomFsTypeUnregister (&fat_fs_type); }
