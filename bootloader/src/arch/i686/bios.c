#include "loom/arch/i686/bios.h"
#include "loom/disk.h"
#include "loom/mm.h"
#include "loom/string.h"

typedef struct
{
  loom_disk_t base;
  loom_uint8_t drive;
} bios_disk_t;

typedef struct
{
  loom_uint16_t size;
  loom_uint16_t flags;
  loom_uint32_t cylinders;
  loom_uint32_t heads;
  loom_uint32_t spt; // sectors per track
  loom_uint64_t sectors;
  loom_uint16_t bps; // bytes per sector
} PACKED bios_disk_params_t;

typedef struct
{
  loom_uint8_t size;
  loom_uint8_t reserved;
  loom_uint16_t blocks;
  loom_uint16_t offset;
  loom_uint16_t segment;
  loom_uint64_t start_block;
} PACKED bios_disk_read_t;

static loom_error_t
bios_disk_read (struct loom_disk_t *disk, loom_usize_t block,
                loom_usize_t count, char *buf)
{
  loom_uint8_t drive = ((bios_disk_t *) disk->data)->drive;
  loom_usize_t length = 0x10000;
  char *bounce = (char *) 0x60000;
  int retries = 0;

  loom_bios_args_t args = { 0 };
  volatile bios_disk_read_t read_packet = { 0 };

  if (block > LOOM_USIZE_MAX - count)
    return LOOM_ERR_OVERFLOW;

  if (block + count > disk->blocks)
    return LOOM_ERR_OVERFLOW;

  if (disk->bpb > length)
    return LOOM_ERR_BAD_BLOCK_SIZE;

  while (count)
    {
      loom_usize_t read = length / disk->bpb, bytes;

      if (count < read)
        read = count;

      if (read > 0x7F)
        read = 0x7F;

      args.eax = 0x42 << 8;
      args.edx = drive;
      args.esi = (loom_address_t) &read_packet;
      args.ds = 0;

      compile_assert (sizeof (bios_disk_read_t) >= 0x10,
                      "bios_disk_read_t must be at least 16 bytes.");
      read_packet.size = 0x10;

      read_packet.blocks = (loom_uint16_t) read;
      read_packet.offset = 0;
      read_packet.segment = (loom_uint16_t) (((loom_address_t) bounce) / 0x10);
      read_packet.start_block = block;

      loom_bios_int (0x13, &args);

      if ((args.flags & 1) || ((args.eax >> 8) & 0xFF)
          || read_packet.blocks != (loom_uint16_t) read)
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

          loom_uint8_t ah = (args.eax >> 8) & 0xFF;

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
      bytes = read * disk->bpb;

      loom_memcpy (buf, bounce, bytes);

      block += read;
      count -= read;
      buf += bytes;
    }

  return LOOM_ERR_NONE;
}

void
loom_bios_disk_probe (void)
{
  loom_bios_args_t args = { 0 };
  volatile bios_disk_params_t params;

  for (loom_uint8_t drive = 0x80; drive <= 0x80; ++drive)
    {
      bios_disk_t *disk;

      args.eax = 0x41 << 8;
      args.ebx = 0x55AA;
      args.edx = drive;

      loom_bios_int (0x13, &args);

      if ((args.flags & 1) || args.ebx != 0xAA55)
        continue;

      args.eax = 0x48 << 8;
      args.edx = drive;
      args.esi = (loom_address_t) &params;
      args.ds = 0;

      compile_assert (sizeof (bios_disk_params_t) >= 0x1A,
                      "bios_disk_params_t must be at least 26 bytes.");
      params.size = 0x1A;

      loom_bios_int (0x13, &args);

      if ((args.flags & 1) || ((args.eax >> 8) & 0xFF))
        continue;

      if (params.sectors == LOOM_UINT64_MAX || !params.bps)
        continue;

      disk = loom_malloc (sizeof (*disk));
      if (!disk)
        continue;

      disk->base = (loom_disk_t) {
        .read = bios_disk_read,
        .bpb = params.bps,
        .blocks = (loom_usize_t) params.sectors,
        .data = disk,
      };

      disk->drive = drive;

      loom_disk_register (&disk->base);
    }
}
