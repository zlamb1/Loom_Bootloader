#ifndef FAT_H
#define FAT_H 1

#include "loom/dir.h"
#include "loom/endian.h"
#include "loom/file.h"
#include "loom/fs.h"

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
#define FAT_DIR_ENTRY_END  0x00
#define FAT_DIR_ENTRY_FREE 0xE5
#define FAT_DIR_ENTRY_E5   0x05
  char file_name[11];
#define FAT_FILE_ATTR_READ_ONLY 0x1
#define FAT_FILE_ATTR_HIDDEN    0x2
#define FAT_FILE_ATTR_SYSTEM    0x4
#define FAT_FILE_ATTR_VOLUME_ID 0x8
#define FAT_FILE_ATTR_DIR       0x10
#define FAT_FILE_ATTR_ARCHIVE   0x20
#define FAT_FILE_ATTR_LFN                                                     \
  (FAT_FILE_ATTR_READ_ONLY | FAT_FILE_ATTR_HIDDEN | FAT_FILE_ATTR_SYSTEM      \
   | FAT_FILE_ATTR_VOLUME_ID)
#define FAT_FILE_ATTR_LFN_MASK                                                \
  (FAT_FILE_ATTR_LFN | FAT_FILE_ATTR_DIR | FAT_FILE_ATTR_ARCHIVE)
  u8 attribs;
  char res1;
  u8 ctime_subsec;
#define FAT_FILE_TIME_SEC(X)  (((X) & 0b11111) * 2)
#define FAT_FILE_TIME_MIN(X)  (((X) & 0b11111100000) >> 5)
#define FAT_FILE_TIME_HOUR(X) (((X) & 0b1111100000000000) >> 11)
  u16le ctime;
#define FAT_FILE_DATE_DAY(X)   ((X) & 0b11111)
#define FAT_FILE_DATE_MONTH(X) (((X) & 0b111100000) >> 5)
#define FAT_FILE_DATE_YEAR(X)  ((((X) & 0b1111111000000000) >> 9) + 1980)
  u16le cdate;
  u16le adate;
  u16le cluster_hi;
  u16le mtime;
  u16le mdate;
  u16le cluster_lo;
  u32le file_size;
} packed fat_dir_entry;

compile_assert (sizeof (fat_dir_entry) == 32, "Bad fat_dir_entry size.");

typedef struct
{
  loom_fs super;
#define FAT_TYPE_12 1
#define FAT_TYPE_16 2
#define FAT_TYPE_32 3
  u16 type;
  u16 reserved_sects;
  u16 bytes_per_sect;
  u16 sects_per_cluster;
  u16 fat_count;
  u32 fat_sects;
  u32 data_off;

  union
  {
    u32 root_cluster;
    u32 root_entry_count;
  };

  struct
  {
    // Scratch buffer with size max(cluster_size, bytes_per_sect*2).
    u32 sect;
    void *buf;
  } scratch;
} fat_fs;

typedef struct
{
  b16 has_next;
  b16 root16_12;
  u32 offset;

  union
  {
    u32 cluster;
    struct
    {
      u32 r_entry;
      u32 count;
    };
  };

  fat_fs *fs;
  fat_dir_entry entry;
} fat_iterator_ctx;

typedef struct
{
  u32 cluster;
  u32 pos;
} fat_file_ctx;

typedef struct
{
  fat_iterator_ctx iterator_ctx;
  loom_dir_entry d_entry;
} fat_dir_ctx;

static inline bool force_inline
fatIsFile (fat_dir_entry *entry)
{
  return !(entry->attribs & (FAT_FILE_ATTR_VOLUME_ID | FAT_FILE_ATTR_DIR));
}

static inline bool force_inline
fatIsDirectory (fat_dir_entry *entry)
{
  return !(entry->attribs & FAT_FILE_ATTR_VOLUME_ID)
         && (entry->attribs & FAT_FILE_ATTR_DIR);
}

static inline u32 force_inline
fatGetClusterLimit (u16 type)
{
  if (type == FAT_TYPE_32)
    return 0x0FFFFFF8;
  else if (type == FAT_TYPE_16)
    return 0xFFF8;
  else
    return 0xFF8;
}

static inline u32 force_inline
fatDirEntryGetCluster (fat_dir_entry *entry)
{
  return (u32) loomEndianLoad (entry->cluster_lo)
         | ((u32) loomEndianLoad (entry->cluster_hi) << 16);
}

static inline u32 force_inline
fatGetClusterSize (fat_fs *fs)
{
  return (u32) fs->bytes_per_sect * (u32) fs->sects_per_cluster;
}

static inline usize force_inline
fatGetClusterOffset (u32 cluster, fat_fs *fs)
{
  return fs->data_off + (cluster - 2) * fatGetClusterSize (fs);
}

static inline usize force_inline
fatGetRootEntriesOffset (fat_fs *fs)
{
  return (fs->reserved_sects + (fs->fat_count * fs->fat_sects))
         * fs->bytes_per_sect;
}

int fatOpen (loom_fs *super, loom_file *file, const char *path);

int fatOpenDir (loom_fs *super, loom_dir *dir, const char *path);

int fatClose (loom_file *file);

int fatCloseDir (loom_dir *dir);

int fatRead (loom_file *file, usize nbytes, void *buf, usize *nread);

loom_dir_entry *fatReadDir (loom_dir *dir);

void fatFree (loom_fs *super);

loom_error fatReadCluster (u32 cluster, void *buf, fat_fs *fs);

loom_error fatNextCluster12 (u32 cluster, u32 *next, void *buf, fat_fs *fs);

loom_error fatNextCluster (u32 cluster, u32 *next, void *buf, fat_fs *fs);

int fatIteratorInit (fat_iterator_ctx *ctx, fat_dir_entry *dir, fat_fs *fs);

int fatIteratorReset (fat_iterator_ctx *ctx, fat_dir_entry *dir);

int fatIterateDirEntries (fat_iterator_ctx *ctx);

int fatFindDirEntry (usize size, const char *name, fat_iterator_ctx *ctx);

/* NAME UTILITIES */

int fatAllocSFN (char **oname, fat_dir_entry *d_entry);

#endif