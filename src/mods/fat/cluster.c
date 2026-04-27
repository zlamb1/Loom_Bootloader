#include "loom/block_dev.h"

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
fatNextCluster12 (u32 cluster, u32 *next, fat_fs *fs)
{
  loom_error error;

  auto offset = (cluster * 12) / 8;
  auto sect = fs->reserved_sects + offset / fs->bytes_per_sect;
  auto sect_offset = offset % fs->bytes_per_sect;
  char p[2];

  if ((error = loomBlockDevRead (
           fs->super.parent, sect * fs->bytes_per_sect + sect_offset, 2, p)))
    return error;

  auto bit_offset = (cluster & 1) ? 4u : 0u;
  u16 v = (u16) p[0] | (u16) (p[1] << 8);

  *next = (v >> bit_offset) & 0xFFF;

  return LOOM_ERR_NONE;
}

loom_error
fatNextCluster (u32 cluster, u32 *next, fat_fs *fs)
{
  loom_error error;

  if (fs->type == FAT_TYPE_12)
    return fatNextCluster12 (cluster, next, fs);

  auto entsz = fs->type == FAT_TYPE_32 ? 4u : 2u;
  auto ents_per_sect = fs->bytes_per_sect / entsz;
  auto entry = cluster % ents_per_sect;
  auto sect = fs->reserved_sects + (entsz * cluster) / fs->bytes_per_sect;

  if (fs->type == FAT_TYPE_32)
    {
      if ((error = loomBlockDevCachedRead (
               fs->super.parent, sect * fs->bytes_per_sect + entry * 4,
               sizeof (*next), (char *) next)))
        return error;
      *next = *next & 0x0FFFFFFF;
    }
  else
    {
      u16 next16;
      if ((error = loomBlockDevCachedRead (
               fs->super.parent, sect * fs->bytes_per_sect + entry * 2,
               sizeof (next16), (char *) &next16)))
        return error;
      *next = next16;
    }

  return LOOM_ERR_NONE;
}