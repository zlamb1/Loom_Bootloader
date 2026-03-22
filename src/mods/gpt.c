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

typedef u64 lba;

typedef struct
{
  lba start;
  lba end;
} packed range;

typedef struct
{
#define GPT_SIGNATURE "EFI PART"
#define GPT_GUID_SIZE 16
  u64 signature;
  u32 revision;
  u32 hdr_sz;
  u32 hdr_crc32;
  u32 res1;
  lba hdr_loc;
  lba backup_loc;
  range alloc;
  char disk_uuid[GPT_GUID_SIZE];
  lba ents_loc;
  u32 ents;
  u32 ents_sz;
  u32 ents_crc32;
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
  u64 attributes;
  char name[72]; // UTF-16LE
} packed gpt_partition_entry;

int
gptPartionSchemeIterate (loom_partition_scheme *partition_scheme,
                         loom_block_dev *parent,
                         loom_partition_scheme_hook hook, void *ctx)
{
  int ret_val = -1;
  gpt_header *hdr = NULL;
  char *ents = NULL;
  loom_error error;
  usize size, loc;

  (void) partition_scheme;

  hdr = loomAlloc (sizeof (*hdr));
  if (hdr == NULL)
    goto out;

  if ((error = loomBlockDevRead (parent, parent->block_size, sizeof (*hdr),
                                 (char *) hdr)))
    {
      loomError (error);
      goto out;
    }

  if (loomMemCmp (&hdr->signature, GPT_SIGNATURE, 8))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT signature");
      goto out;
    }

  hdr->hdr_sz = le32toh (hdr->hdr_sz);

  if (hdr->hdr_sz < sizeof (*hdr))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT header size");
      goto out;
    }

  // FIXME: validate header CRC32

  hdr->ents_sz = le32toh (hdr->ents_sz);

  if (hdr->ents_sz < sizeof (gpt_partition_entry))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "bad GPT entry size");
      goto out;
    }

  hdr->ents = le32toh (hdr->ents);

  if (loomMul (hdr->ents, hdr->ents_sz, &size))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME, "GPT table entries overflow");
      goto out;
    }

  hdr->ents_loc = le64toh (hdr->ents_loc);

  if (loomMul (parent->block_size, hdr->ents_loc, &loc))
    {
      loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME,
                    "GPT table entries out of range");
      goto out;
    }

  ents = loomAlloc (size);

  if (ents == NULL)
    goto out;

  if ((error = loomBlockDevRead (parent, loc, size, ents)))
    {
      loomError (error);
      goto out;
    }

  // TODO: validate entries CRC32

  for (usize i = 0; i < hdr->ents; ++i)
    {
      loom_partition partition;
      gpt_partition_entry *ent
          = (gpt_partition_entry *) (ents + i * hdr->ents_sz);
      usize offset, blocks;

      if (!loomMemCmp (ent->partition_type, gpt_part_type_unused,
                       GPT_GUID_SIZE))
        continue;

      ent->range.start = le64toh (ent->range.start);
      ent->range.end = le64toh (ent->range.end);

      if (ent->range.start > ent->range.end)
        {
          loomErrorFmt (LOOM_ERR_BAD_PART_SCHEME,
                        "bad GPT partition entry range");
          goto out;
        }

      if (loomCheckedCast (ent->range.start, &offset)
          || loomAdd (ent->range.end - offset, 1, &blocks))
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
  loomFree (ents);
  loomFree (hdr);

  return ret_val;
}

LOOM_MOD (gpt)

LOOM_MOD_INIT () { loomPartitionSchemeRegister (&gpt_partition_scheme); }

LOOM_MOD_DEINIT () { loomPartitionSchemeUnregister (&gpt_partition_scheme); }