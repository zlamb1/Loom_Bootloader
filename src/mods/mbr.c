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
  u32le start;
  u32le sectors;
} packed mbr_partition_entry;

struct packed mbr
{
#define MBR_SIZE 512
  char pad[446];
  mbr_partition_entry entries[4];
  u16le signature;
};

static int mbrPartitionSchemeIterate (loom_partition_scheme *,
                                      loom_block_dev *,
                                      loom_partition_scheme_hook, void *);

loom_partition_scheme mbr_partition_scheme = {
  .iterate = mbrPartitionSchemeIterate,
  .name = "mbr",
};

int
mbrPartitionSchemeIterate (loom_partition_scheme *partition_scheme,
                           loom_block_dev *parent,
                           loom_partition_scheme_hook hook, void *ctx)
{
  int ret_val = -1;
  struct mbr *mbr = loomAlloc (sizeof (struct mbr));
  loom_error error;

  compile_assert (sizeof (struct mbr) == MBR_SIZE, "bad size for mbr_t");

  (void) partition_scheme;

  if (mbr == NULL)
    goto out;

  if ((error = loomBlockDevRead (parent, 0, MBR_SIZE, (char *) mbr)))
    goto out;

  auto signature = loomEndianLoad (mbr->signature);

  if (signature != 0xAA55)
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad MBR signature '0x%.4lx'",
                    (unsigned long) signature);
      goto out;
    }

  for (int i = 0; i < 4; ++i)
    {
      loom_partition partition;
      mbr_partition_entry *entry = mbr->entries + i;

      if (entry->status & 0x7F)
        {
          loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME,
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
          loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "protective MBR");
          goto out;
        }

      if (loomPartitionInit (&partition, parent, loomEndianLoad (entry->start),
                             loomEndianLoad (entry->sectors)))
        goto out;

      if ((ret_val = hook (parent, &partition, ctx)))
        {
          loomError (LOOM_ERR_HOOK);
          goto out;
        }
    }

  ret_val = 0;

out:
  loomFree (mbr);
  return ret_val;
}

LOOM_MOD (mbr)

LOOM_MOD_INIT () { loomPartitionSchemeRegister (&mbr_partition_scheme); }

LOOM_MOD_DEINIT () { loomPartitionSchemeUnregister (&mbr_partition_scheme); }