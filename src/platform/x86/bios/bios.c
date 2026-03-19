#include "loom/platform/x86/bios/bios.h"
#include "loom/assert.h"
#include "loom/block_dev.h"
#include "loom/compiler.h"
#include "loom/mm.h"
#include "loom/string.h"

typedef struct
{
  loom_block_dev bd;
  u8 drive;
} bios_disk;

typedef struct
{
  u16 size;
  u16 flags;
  u32 cylinders;
  u32 heads;
  u32 spt; // sectors per track
  u64 sectors;
  u16 bps; // bytes per sector
} packed bios_disk_params;

typedef struct
{
  u8 size;
  u8 reserved;
  u16 blocks;
  u16 offset;
  u16 segment;
  u64 start_block;
} packed bios_disk_read_packet;

static loom_error
biosDiskRead (loom_block_dev *block_dev, usize block, usize count, char *buf)
{
  loomAssert (block_dev != NULL);
  loomAssert (block_dev->data != NULL);

  u8 drive = ((bios_disk *) block_dev->data)->drive;
  usize length = 0x10000, block_size;
  char *bounce = (char *) 0x60000;
  int retries = 0;

  loom_bios_args args = { 0 };
  volatile bios_disk_read_packet read_packet = { 0 };

  if (block > USIZE_MAX - count)
    return LOOM_ERR_OVERFLOW;

  if (block + count > block_dev->blocks)
    return LOOM_ERR_OVERFLOW;

  block_size = block_dev->block_size;
  if (block_size > length)
    return LOOM_ERR_BAD_BLOCK_SIZE;

  while (count)
    {
      usize read = length / block_size, bytes;

      if (count < read)
        read = count;

      if (read > 0x7F)
        read = 0x7F;

      args.eax = 0x42 << 8;
      args.edx = drive;
      args.esi = (address) &read_packet;
      args.ds = 0;

      compile_assert (sizeof (bios_disk_read_packet) >= 0x10,
                      "bios_disk_read_t must be at least 16 bytes.");
      read_packet.size = 0x10;

      read_packet.blocks = (u16) read;
      read_packet.offset = 0;
      read_packet.segment = (u16) (((address) bounce) / 0x10);
      read_packet.start_block = block;

      loomBiosInt (0x13, &args);

      if ((args.flags & 1) || ((args.eax >> 8) & 0xFF)
          || read_packet.blocks != (u16) read)
        {
#define READ_ERROR         0x04
#define RESET_FAILED       0x05
#define DMA_OVERRUN        0x08
#define CONTROLLER_FAILURE 0x20
#define SEEK_FAIL          0x40
#define NOT_READY          0x80
#define DRIVE_NOT_READY    0xAA
#define VOLUME_IN_USE      0xB3
#define UNDEFINED_ERROR    0xBB

          u8 ah = (args.eax >> 8) & 0xFF;

          if ((args.flags & 1)
              && (ah == READ_ERROR || ah == RESET_FAILED || ah == DMA_OVERRUN
                  || ah == CONTROLLER_FAILURE || ah == SEEK_FAIL
                  || ah == NOT_READY || ah == DRIVE_NOT_READY
                  || ah == VOLUME_IN_USE || ah == UNDEFINED_ERROR)
              && retries < 3)
            {
              ++retries;
              continue;
            }

          return LOOM_ERR_IO;
        }

      retries = 0;
      bytes = read * block_size;

      loomMemCopy (buf, bounce, bytes);

      block += read;
      count -= read;
      buf += bytes;
    }

  return LOOM_ERR_NONE;
}

void
loomBiosDisksProbe (void)
{
  loom_bios_args args = { 0 };
  volatile bios_disk_params params;

  for (u8 drive = 0x80; drive <= 0x8F; ++drive)
    {
      bios_disk *disk;

      args.eax = 0x41 << 8;
      args.ebx = 0x55AA;
      args.edx = drive;

      loomBiosInt (0x13, &args);

      if ((args.flags & 1) || args.ebx != 0xAA55)
        continue;

      args.eax = 0x48 << 8;
      args.edx = drive;
      args.esi = (address) &params;
      args.ds = 0;

      compile_assert (sizeof (bios_disk_params) >= 0x1A,
                      "bios_disk_params_t must be at least 26 bytes.");
      params.size = 0x1A;

      loomBiosInt (0x13, &args);

      if ((args.flags & 1) || ((args.eax >> 8) & 0xFF))
        continue;

      if (params.sectors == U64_MAX || !params.bps)
        continue;

      disk = loomAlloc (sizeof (*disk));
      if (!disk)
        continue;

      disk->bd = (loom_block_dev) {
        .read = biosDiskRead,
        .block_size = params.bps,
        .blocks = (usize) params.sectors,
        .data = disk,
      };

      loom_block_dev_init_t init = {
        .read = biosDiskRead,
        .block_size = params.bps,
        .blocks = (usize) params.sectors,
        .data = disk,
      };

      loomBlockDevInit (&disk->bd, &init);
      disk->drive = drive;

      loomBlockDevRegister (&disk->bd);
    }
}
