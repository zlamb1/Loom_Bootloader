#include "loom/block_dev.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/fs.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"

typedef struct
{
#define FAT_EBPB16                                                            \
  u8 drive_num;                                                               \
  u8 nt_flags;                                                                \
  u8 sig;                                                                     \
  u32le volume_id;                                                            \
  char volume_label[11];                                                      \
  char sys_ident[8];
#define FAT_EBPB_SIG1 0x28
#define FAT_EBPB_SIG2 0x29
  FAT_EBPB16
} packed fat_ebpb16;

typedef struct
{
  u32le fatsz32;
  u16le flags;
  u16le version;
  u32le root_cluster;
  u16le fsinfo_sect;
  u16le backup_sect;
  char res1[12];
  FAT_EBPB16
} packed fat_ebpb32;

typedef struct
{
  u8 jmp[3];
  char oem_ident[8];
  u16le bytes_per_sect;
  u8 sects_per_cluster;
  u16le reserved_sects;
  u8 fats;
  u16le root_entry_count;
  u16le sects16;
  u8 media_desc;
  u16le fatsz16;
  u16le sects_per_track;
  u16le heads;
  u32le hidden_sects;
  u32le sects32;

  union
  {
    fat_ebpb16 ebpb16;
    fat_ebpb32 ebpb32;
  };

  u8 pad[420];
#define FAT_BOOT_SIGNATURE 0xAA55
  u16le signature;
} packed fat_bpb;

typedef struct
{
#define FAT_FSINFO_SIG1 0x41615252
  u32le sig1;
  byte res1[480];
#define FAT_FSINFO_SIG2 0x61417272
  u32le sig2;
  u32le free_count_hint;
  u32le free_cluster_hint;
  byte res2[12];
#define FAT_FSINFO_SIG3 0xAA550000
  u32le sig3;
} packed fat_fsinfo;

typedef struct
{
  char filename[11];
#define FAT_FILE_ATTR_READ_ONLY 0x1
#define FAT_FILE_ATTR_HIDDEN    0x2
#define FAT_FILE_ATTR_SYSTEM    0x4
#define FAT_FILE_ATTR_VOLUME_ID 0x8
#define FAT_FILE_ATTR_DIR       0x10
#define FAT_FILE_ATTR_ARCHIVE   0x20
#define FAT_FILE_ATTR_LFN                                                     \
  (FAT_FILE_ATTR_READ_ONLY | FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM      \
   | FAT_FILE_ATTR_VOLUME_ID)
  u8 attribs;
  char res1;
  u8 ctime_subsec;
#define FAT_FILE_TIME_SEC(X)  (((X) & 0b11111) * 2)
#define FAT_FILE_TIME_MIN(X)  (((X) & 0b11111100000) >> 5)
#define FAT_FILE_TIME_HOUR(X) (((X) & 0b1111100000000000) >> 11)
  u16 ctime;
#define FAT_FILE_DATE_DAY(X)   ((X) & 0b11111)
#define FAT_FILE_DATE_MONTH(X) (((X) & 0b111100000) >> 5)
#define FAT_FILE_DATE_YEAR(X)  ((((X) & 0b1111111000000000) >> 9) + 1980)
  u16 cdate;
  u16 adate;
  u16 cluster_hi;
  u16 mtime;
  u16 mdate;
  u16 cluster_lo;
  u32 file_size;
} packed fat_dirent;

typedef struct
{
  loom_fs super;
#define FAT_TYPE_12 1
#define FAT_TYPE_16 2
#define FAT_TYPE_32 3
  u16 type;
  u16 bytes_per_sect;
  u16 sects_per_cluster;
} fat_fs;

static inline loom_fs *fat_probe (loom_block_dev *block_dev);

loom_fs_type fat_fs_type = {
  .name = "fat",
  .probe = fat_probe,
};

LOOM_MOD (fat)

static inline loom_fs *
fat_probe (loom_block_dev *block_dev)
{
  fat_bpb bpb;
  fat_fs *fs = loomAlloc (sizeof (*fs));

  if (fs == null)
    return null;

  if (loomBlockDevRead (block_dev, 0, sizeof (bpb), (char *) &bpb))
    goto out;

  auto signature = endianLoad (bpb.signature);
  if (signature != FAT_BOOT_SIGNATURE)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad boot signature 0x%.4lx",
                    (ulong) signature);
      goto out;
    }

  auto bytes_per_sect = endianLoad (bpb.bytes_per_sect);
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

  auto reserved_sects = endianLoad (bpb.reserved_sects);
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

  auto root_entry_count = endianLoad (bpb.root_entry_count);
  auto fatsz16 = endianLoad (bpb.fatsz16);
  auto sects16 = endianLoad (bpb.sects16);

  u32 root_dir_sects = (((u32) root_entry_count * 32) + (bytes_per_sect - 1))
                       / bytes_per_sect;
  u32 fatsz;
  u32 sects;

  if (fatsz16)
    fatsz = fatsz16;
  else
    fatsz = endianLoad (bpb.ebpb32.fatsz32);

  if (!fatsz)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad FAT size %lu", (ulong) fatsz);
      goto out;
    }

  if (sects16)
    sects = sects16;
  else
    sects = endianLoad (bpb.sects32);

  if (!sects)
    {
      loomErrorFmt (LOOM_ERR_BAD_FS, "bad number of sectors %lu",
                    (ulong) sects);
      goto out;
    }

  u32 meta_sects;
  if (loomMul (fat_count, fatsz, &meta_sects)
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

  fs->super.data = fs;

  fs->type = fat_type;
  fs->bytes_per_sect = bytes_per_sect;
  fs->sects_per_cluster = sects_per_cluster;

  return &fs->super;

out:
  loomFree (fs);
  return null;
}

LOOM_MOD_INIT () { loomFsTypeRegister (&fat_fs_type); }

LOOM_MOD_DEINIT () { loomFsTypeUnregister (&fat_fs_type); }
