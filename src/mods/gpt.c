#include "loom/block_dev.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/string.h"

int gptPartionSchemeIterate (loom_partition_scheme *, loom_block_dev *,
                             loom_partition_scheme_hook, void *);

static loom_partition_scheme gpt_partition_scheme = {
  .iterate = gptPartionSchemeIterate,
};

typedef u64le lba;

typedef struct
{
  lba start;
  lba end;
} packed range;

typedef struct
{
#define GPT_SIGNATURE "EFI PART"
#define GPT_GUID_SIZE 16
  char signature[8];
  u32le revision;
  u32le size;
  u32le crc32;
  u32le res1;
  lba loc;
  lba backup_loc;
  range alloc;
  char disk_uuid[GPT_GUID_SIZE];
  lba entry_loc;
  u32le entry_count;
  u32le entry_size;
  u32le entry_crc32;
} packed gpt_header;

static char gpt_part_type_unused[GPT_GUID_SIZE] = { 0 };

typedef struct
{
  char partition_type[GPT_GUID_SIZE];
  char partition_uuid[GPT_GUID_SIZE];
  range range;
#define GPT_PART_PLATFORM_REQ  (1 << 0)
#define GPT_PART_EFI_IGNORE    (1 << 1)
#define GPT_PART_BIOS_BOOTABLE (1 << 2)
  u64le attributes;
  char name[72]; // UTF-16LE
} packed gpt_partition_entry;

int
gptPartionSchemeIterate (loom_partition_scheme *partition_scheme,
                         loom_block_dev *parent,
                         loom_partition_scheme_hook hook, void *ctx)
{
  int ret_val = -1;
  gpt_header *header = NULL;
  char *entry_table = NULL;
  usize entry_table_size, loc;
  loom_error error;

  (void) partition_scheme;

  header = loomAlloc (sizeof (*header));
  if (header == NULL)
    goto out;

  if ((error = loomBlockDevRead (parent, parent->block_size, sizeof (*header),
                                 (char *) header)))
    {
      loomError (error);
      goto out;
    }

  if (loomMemCmp (&header->signature, GPT_SIGNATURE, 8))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT signature");
      goto out;
    }

  if (loomEndianLoad (header->size) < sizeof (*header))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT header size");
      goto out;
    }

  // FIXME: validate header CRC32

  auto entry_size = loomEndianLoad (header->entry_size);

  if (entry_size < sizeof (gpt_partition_entry))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT entry size");
      goto out;
    }

  auto entry_count = loomEndianLoad (header->entry_count);

  if (loomMul (entry_count, entry_size, &entry_table_size))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "GPT table entries overflow");
      goto out;
    }

  if (loomMul (parent->block_size, loomEndianLoad (header->entry_loc), &loc))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME,
                    "GPT table entries out of range");
      goto out;
    }

  entry_table = loomAlloc (entry_table_size);

  if (entry_table == NULL)
    goto out;

  if ((error = loomBlockDevRead (parent, loc, entry_table_size, entry_table)))
    {
      loomError (error);
      goto out;
    }

  // TODO: validate entries CRC32

  for (usize i = 0; i < entry_count; ++i)
    {
      loom_partition partition;
      gpt_partition_entry *ent
          = (gpt_partition_entry *) (entry_table + i * entry_size);
      usize offset, blocks;

      if (!loomMemCmp (ent->partition_type, gpt_part_type_unused,
                       GPT_GUID_SIZE))
        continue;

      auto start = loomEndianLoad (ent->range.start);
      auto end = loomEndianLoad (ent->range.end);

      if (start > end)
        {
          loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME,
                        "bad GPT partition entry range");
          goto out;
        }

      if (loomCheckedCast (start, &offset)
          || loomAdd (end - offset, 1, &blocks))
        // FIXME: Emit warning for out of range partition.
        continue;

      loomPartitionInit (&partition, parent, offset, blocks);

      if ((ret_val = hook (parent, &partition, ctx)))
        {
          loomError (LOOM_ERR_HOOK);
          goto out;
        }
    }

  ret_val = 0;
out:
  loomFree (entry_table);
  loomFree (header);

  return ret_val;
}

LOOM_MOD (gpt)

LOOM_MOD_INIT () { loomPartitionSchemeRegister (&gpt_partition_scheme); }

LOOM_MOD_DEINIT () { loomPartitionSchemeUnregister (&gpt_partition_scheme); }