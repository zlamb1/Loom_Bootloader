#include "loom/module.h"

typedef struct
{
#define FAT_EBPB16                                                            \
  u8 drive_num;                                                               \
  u8 nt_flags;                                                                \
  u8 sig;                                                                     \
  u32 volume_id;                                                              \
  char volume_label[11];                                                      \
  char sys_ident[8];
#define FAT_EBPB_SIG1 0x28
#define FAT_EBPB_SIG2 0x29
  FAT_EBPB16
} packed fat_ebpb16;

typedef struct
{
  u32 fatsz32;
  u16 flags;
  u16 version;
  u32 root_cluster;
  u16 fsinfo_sect;
  u16 backup_sect;
  char res1[12];
  FAT_EBPB16
} packed fat_ebpb32;

typedef struct
{
  u8 jmp[3];
  char oem_ident[8];
  u16 bytes_per_sect;
  u8 sects_per_cluster;
  u16 start_sectors;
  u8 fats;
  u16 root_dents;
  u16 sects16;
  u8 media_desc;
  u16 fatsz16;
  u16 sects_per_track;
  u16 heads;
  u32 hidden_sects;
  u32 sects32;

  union
  {
    fat_ebpb16 ebpb16;
    fat_ebpb32 ebpb32;
  };
} packed fat_bpb;

typedef struct
{
#define FAT_FSINFO_SIG1 0x41615252
  u32 sig1;
  byte res1[480];
#define FAT_FSINFO_SIG2 0x61417272
  u32 sig2;
  u32 free_count_hint;
  u32 free_cluster_hint;
  byte res2[12];
#define FAT_FSINFO_SIG3 0xAA550000
  u32 sig3;
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

LOOM_MOD (fat)

LOOM_MOD_INIT () {}

LOOM_MOD_DEINIT () {}
