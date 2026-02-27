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
  loom_uint8_t status;
  char chs_start[3];
#define PART_TYPE_EMPTY 0x00
#define PART_TYPE_GPT   0xEE
  loom_uint8_t partition_type;
  char chs_end[3];
  loom_uint32_t lba_start;
  loom_uint32_t sectors;
} LOOM_PACKED mbr_partition_entry_t;

typedef struct
{
#define MBR_SIZE 512
  char pad[446];
  mbr_partition_entry_t entries[4];
  loom_uint16_t signature;
} LOOM_PACKED mbr_t;

static int
mbr_partition_scheme_iterate (loom_partition_scheme_t *partition_scheme,
                              loom_block_dev_t *parent,
                              loom_partition_scheme_hook_t hook);

loom_partition_scheme_t mbr_partition_scheme = {
  .iterate = mbr_partition_scheme_iterate,
};

int
mbr_partition_scheme_iterate (loom_partition_scheme_t *partition_scheme,
                              loom_block_dev_t *parent,
                              loom_partition_scheme_hook_t hook)
{
  int retval = -1;
  mbr_t *mbr = loom_malloc (sizeof (mbr_t));
  loom_error_t error;

  compile_assert (sizeof (mbr_t) == MBR_SIZE, "bad size for mbr_t");

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
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "bad MBR signature '%lx'",
                  (unsigned long) mbr->signature);
      goto out;
    }

  for (int i = 0; i < 4; ++i)
    {
      loom_partition_t partition;

      mbr_partition_entry_t *entry = mbr->entries + i;

      if (entry->status & 0x7F)
        {
          loom_error (LOOM_ERR_BAD_PART_SCHEME,
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
          loom_error (LOOM_ERR_BAD_PART_SCHEME, "protective MBR");
          goto out;
        }

      entry->lba_start = loom_le32toh (entry->lba_start);
      entry->sectors = loom_le32toh (entry->sectors);

      loom_partition_init (&partition, parent, entry->lba_start,
                           entry->sectors);

      if ((retval = hook (parent, &partition)))
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