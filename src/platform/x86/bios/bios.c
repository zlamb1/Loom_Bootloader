#include "loom/platform/x86/bios/bios.h"
#include "loom/assert.h"
#include "loom/block_dev.h"
#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/platform.h"
#include "loom/string.h"
#include "loom/types.h"

typedef struct
{
  loom_block_dev super;
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
  u64 start_block; // 0
  u16 blocks;      // 8
  u16 off;         // 10
  u16 seg;         // 12
  u8 drive;        // 14
  u8 pad[1];
} packed bios_io_req;

compile_assert (sizeof (bios_io_req) == 16, "bad bios_io_req size");

extern bios_io_req bios_io_reqs[BIOS_IO_REQS_LIMIT];

int biosDiskReadImpl (u16 count);

static inline int
biosDiskReadImplWrapper (u16 count)
{
  int flags = loomIrqSave ();
  loomRunBiosEnterHooks ();
  int ret_val = biosDiskReadImpl (count);
  loomRunBiosLeaveHooks ();
  loomIrqRestore (flags);
  return ret_val;
}

static loom_error
biosDiskRead (loom_block_dev *block_dev, loom_io_req *io_reqs)
{
  loomAssert (block_dev != null);
  loomAssert (block_dev->data != null);
  loomAssert (block_dev->block_size > 0);
  loomAssert (!(block_dev->block_size % 16));

  auto disk = (bios_disk *) block_dev->data;
  auto drive = disk->drive;

  usize block_size = block_dev->block_size;
  usize length = 0x10000;

  /* Arbitrary choice. Well below extended BIOS data area. Needs to be real
   * mode accessible. (e.g. below first MiB) */
  char *bounce = (char *) 0x60000;

  /* 0x7F = maximum count for some BIOS implementations. */
  const usize max_allotment = loomMin (0x7F, length / block_size);
  loomAssert (max_allotment > 0);

  loom_io_req partial;

  while (io_reqs != null)
    {
      char *bounce_next = bounce;
      usize request_count = 0;

      auto iter = io_reqs;

      for (usize i = 0, allotment = max_allotment;
           iter != null && i < BIOS_IO_REQS_LIMIT && allotment; i++)
        {
          auto blocks = loomMin (iter->count, allotment);

          loomAssert ((address) bounce_next / 0x10 <= U16_MAX);

          bios_io_reqs[request_count++] = (bios_io_req) {
            .start_block = iter->block,
            .blocks = (u16) blocks,
            .off = 0,
            .seg = (u16) (((address) bounce_next) / 0x10),
            .drive = drive,
          };

          allotment -= (usize) blocks;
          bounce_next += blocks * block_size;

          iter = iter->next;
        }

      loomAssert (request_count <= U16_MAX);
      if (biosDiskReadImplWrapper ((u16) request_count))
        return LOOM_ERR_IO;

#ifdef LOOM_DEBUG
      block_dev->read_count++;
#endif

      bounce_next = bounce;
      iter = io_reqs;

      /* Consume requests and copy from the bounce buffer. */
      for (usize i = 0, allotment = max_allotment;
           iter != null && i < BIOS_IO_REQS_LIMIT && allotment; i++)
        {
          auto blocks = loomMin (iter->count, allotment);

          /* Do the copy. */
          loomMemCopy (iter->buf, bounce_next, (usize) blocks * block_size);

          allotment -= (usize) blocks;
          bounce_next += blocks * block_size;

          /* Consume the I/O requests. */
          if (iter->count > blocks)
            {
              /* This is necessary to prevent modifying user IO requests
               * in-place. */
              if (iter != &partial)
                partial = *iter;

              partial.block += blocks;
              partial.count -= blocks;
              partial.buf = (char *) iter->buf + blocks * block_size;

              iter = &partial;
            }
          else
            iter = iter->next;
        }

      io_reqs = iter;
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
                      "bios_disk_params must be at least 26 bytes.");
      params.size = 0x1A;

      loomBiosInt (0x13, &args);

      if ((args.flags & 1) || ((args.eax >> 8) & 0xFF))
        continue;

      if (params.sectors == U64_MAX || !params.bps)
        continue;

      disk = loomAlloc (sizeof (*disk));
      if (!disk)
        continue;

      disk->super = (loom_block_dev) {
        .readv = biosDiskRead,
        .block_size = params.bps,
        .blocks = (usize) params.sectors,
        .data = disk,
      };

      loom_block_dev_init_t init = {
        .readv = biosDiskRead,
        .block_size = params.bps,
        .blocks = (usize) params.sectors,
        .data = disk,
      };

      if (loomBlockDevInit (&disk->super, &init))
        loomPanic ("Failed to initialize bios disk.");
      disk->drive = drive;

      loomBlockDevRegister (&disk->super);
    }
}
