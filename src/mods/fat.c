#include "loom/module.h"

typedef struct
{
#define FAT_EBPB16                                                            \
  loom_uint8_t drive_num;                                                     \
  loom_uint8_t nt_flags;                                                      \
  loom_uint8_t sig;                                                           \
  loom_uint32_t volume_id;                                                    \
  char volume_label[11];                                                      \
  char sys_ident[8];
#define FAT_EBPB_SIG1 0x28
#define FAT_EBPB_SIG2 0x29
  FAT_EBPB16
} LOOM_PACKED fat_ebpb16_t;

typedef struct
{
  loom_uint32_t fatsz32;
  loom_uint16_t flags;
  loom_uint16_t version;
  loom_uint32_t root_cluster;
  loom_uint16_t fsinfo_sect;
  loom_uint16_t backup_sect;
  char res1[12];
  FAT_EBPB16
} LOOM_PACKED fat_ebpb32_t;

typedef struct
{
  loom_uint8_t jmp[3];
  char oem_ident[8];
  loom_uint16_t bytes_per_sect;
  loom_uint8_t sects_per_cluster;
  loom_uint16_t start_sectors;
  loom_uint8_t fats;
  loom_uint16_t root_dents;
  loom_uint16_t sects16;
  loom_uint8_t media_desc;
  loom_uint16_t fatsz16;
  loom_uint16_t sects_per_track;
  loom_uint16_t heads;
  loom_uint32_t hidden_sects;
  loom_uint32_t sects32;

  union
  {
    fat_ebpb16_t ebpb16;
    fat_ebpb32_t ebpb32;
  };
} LOOM_PACKED fat_bpb_t;

typedef struct
{
#define FAT_FSINFO_SIG1 0x41615252
  loom_uint32_t sig1;
  char res1[480];
#define FAT_FSINFO_SIG2 0x61417272
  loom_uint32_t sig2;
  loom_uint32_t free_count_hint;
  loom_uint32_t free_cluster_hint;
  char res2[12];
#define FAT_FSINFO_SIG3 0xAA550000
  loom_uint32_t sig3;
} LOOM_PACKED fat_fsinfo_t;

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
  loom_uint8_t attribs;
  char res1;
  loom_uint8_t ctime_subsec;
#define FAT_FILE_TIME_SEC(X)  (((X) & 0b11111) * 2)
#define FAT_FILE_TIME_MIN(X)  (((X) & 0b11111100000) >> 5)
#define FAT_FILE_TIME_HOUR(X) (((X) & 0b1111100000000000) >> 11)
  loom_uint16_t ctime;
#define FAT_FILE_DATE_DAY(X)   ((X) & 0b11111)
#define FAT_FILE_DATE_MONTH(X) (((X) & 0b111100000) >> 5)
#define FAT_FILE_DATE_YEAR(X)  ((((X) & 0b1111111000000000) >> 9) + 1980)
  loom_uint16_t cdate;
  loom_uint16_t adate;
  loom_uint16_t cluster_hi;
  loom_uint16_t mtime;
  loom_uint16_t mdate;
  loom_uint16_t cluster_lo;
  loom_uint32_t filesz;
} LOOM_PACKED fat_dirent_t;

LOOM_MOD (fat)

LOOM_MOD_INIT () {}

LOOM_MOD_DEINIT () {}
