#include "fat.h"

loom_error
fatReadCluster (u32 cluster, void *buf, fat_fs *fs)
{
  auto bytes_per_cluster
      = (u32) fs->bytes_per_sect * (u32) fs->sects_per_cluster;
  auto offset = (fs->reserved_sects + fs->fat_count * fs->fat_sects)
                * fs->bytes_per_sect;
  return loomBlockDevRead (fs->super.parent,
                           offset + (cluster - 2) * bytes_per_cluster,
                           bytes_per_cluster, buf);
}

loom_error
fatNextCluster12 (u32 cluster, u32 *next, void *buf, fat_fs *fs)
{
  loom_error error;

  auto offset = (cluster * 12) / 8;
  auto sect = fs->reserved_sects + offset / fs->bytes_per_sect;
  auto sect_offset = offset % fs->bytes_per_sect;

  if ((error = loomBlockDevRead (fs->super.parent, sect * fs->bytes_per_sect,
                                 fs->bytes_per_sect * 2, buf)))
    return error;

  auto p = (uchar *) buf + sect_offset;
  auto bit_offset = (cluster & 1) ? 4u : 0u;
  u16 v = (u16) p[0] | (u16) (p[1] << 8);

  *next = (v >> bit_offset) & 0xFFF;

  return LOOM_ERR_NONE;
}

loom_error
fatNextCluster (u32 cluster, u32 *next, void *buf, fat_fs *fs)
{
  loom_error error;

  if (fs->type == FAT_TYPE_12)
    return fatNextCluster12 (cluster, next, buf, fs);

  auto entsz = fs->type == FAT_TYPE_32 ? 4u : 2u;
  auto ents_per_sect = fs->bytes_per_sect / entsz;
  auto entry = cluster % ents_per_sect;
  auto sect = fs->reserved_sects + (entsz * cluster) / fs->bytes_per_sect;

  if ((error = loomBlockDevRead (fs->super.parent, sect * fs->bytes_per_sect,
                                 fs->bytes_per_sect, buf)))
    return error;

  if (fs->type == FAT_TYPE_32)
    *next = ((u32 *) buf)[entry] & 0x0FFFFFFF;
  else if (fs->type == FAT_TYPE_16)
    *next = ((u16 *) buf)[entry];

  return LOOM_ERR_NONE;
}