#include "loom/block_dev.h"
#include "loom/compiler.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"

typedef struct
{
  u8 status;
  char chs_start[3];
#define PART_TYPE_EMPTY 0x00
#define PART_TYPE_GPT   0xEE
  u8 partition_type;
  char chs_end[3];
  u32 lba_start;
  u32 sectors;
} LOOM_PACKED mbr_partition_entry;

struct LOOM_PACKED mbr
{
#define MBR_SIZE 512
  char pad[446];
  mbr_partition_entry entries[4];
  u16 signature;
};

static int mbr_partition_scheme_iterate (loom_partition_scheme *,
                                         loom_block_dev *,
                                         loom_partition_scheme_hook, void *);

loom_partition_scheme mbr_partition_scheme = {
  .iterate = mbr_partition_scheme_iterate,
};

int
mbr_partition_scheme_iterate (loom_partition_scheme *partition_scheme,
                              loom_block_dev *parent,
                              loom_partition_scheme_hook hook, void *ctx)
{
  int retval = -1;
  struct mbr *mbr = loom_malloc (sizeof (struct mbr));
  loom_error error;

  loom_compile_assert (sizeof (struct mbr) == MBR_SIZE, "bad size for mbr_t");

  (void) partition_scheme;

  if (mbr == NULL)
    goto out;

  if ((error = loom_block_dev_read (parent, 0, MBR_SIZE, (char *) mbr)))
    {
      loom_error_np (error);
      goto out;
    }

  mbr->signature = loom_le16toh (mbr->signature);

  if (mbr->signature != 0xAA55)
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "bad MBR signature '%lx'",
                      (unsigned long) mbr->signature);
      goto out;
    }

  for (int i = 0; i < 4; ++i)
    {
      loom_partition partition;
      mbr_partition_entry *entry = mbr->entries + i;

      if (entry->status & 0x7F)
        {
          loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME,
                          "invalid MBR partition entry status");
          goto out;
        }

      if (entry->partition_type == PART_TYPE_EMPTY)
        continue;

      if (entry->partition_type == 0x05 || entry->partition_type == 0x0F
          || entry->partition_type == 0x85)
        // Note: For now, we don't support extended partitions.
        continue;

      if (entry->partition_type == PART_TYPE_GPT)
        {
          loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "protective MBR");
          goto out;
        }

      entry->lba_start = loom_le32toh (entry->lba_start);
      entry->sectors = loom_le32toh (entry->sectors);

      loom_partition_init (&partition, parent, entry->lba_start,
                           entry->sectors);

      if ((retval = hook (parent, &partition, ctx)))
        {
          loom_error_np (LOOM_ERR_HOOK);
          goto out;
        }
    }

  retval = 0;

out:
  loom_free (mbr);
  return retval;
}

LOOM_MOD (mbr)

LOOM_MOD_INIT () { loom_partition_scheme_register (&mbr_partition_scheme); }

LOOM_MOD_DEINIT ()
{
  loom_partition_scheme_unregister (&mbr_partition_scheme);
}