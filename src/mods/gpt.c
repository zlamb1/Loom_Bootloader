#include "loom/block_dev.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/string.h"

int gpt_partition_scheme_iterate (loom_partition_scheme *, loom_block_dev *,
                                  loom_partition_scheme_hook, void *);

static loom_partition_scheme gpt_partition_scheme = {
  .iterate = gpt_partition_scheme_iterate,
};

typedef u64 lba;

typedef struct
{
  lba start;
  lba end;
} LOOM_PACKED range;

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
} LOOM_PACKED gpt_header;

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
} LOOM_PACKED gpt_partition_entry;

int
gpt_partition_scheme_iterate (loom_partition_scheme *partition_scheme,
                              loom_block_dev *parent,
                              loom_partition_scheme_hook hook, void *ctx)
{
  int ret_val = -1;
  gpt_header *hdr = NULL;
  char *ents = NULL;
  loom_error error;
  usize size, loc;

  (void) partition_scheme;

  hdr = loom_malloc (sizeof (*hdr));
  if (hdr == NULL)
    goto out;

  if ((error = loom_block_dev_read (parent, parent->block_size, sizeof (*hdr),
                                    (char *) hdr)))
    {
      loom_error_np (error);
      goto out;
    }

  if (loom_memcmp (&hdr->signature, GPT_SIGNATURE, 8))
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT signature");
      goto out;
    }

  hdr->hdr_sz = loom_le32toh (hdr->hdr_sz);

  if (hdr->hdr_sz < sizeof (*hdr))
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT header size");
      goto out;
    }

  // FIXME: validate header CRC32

  hdr->ents_sz = loom_le32toh (hdr->ents_sz);

  if (hdr->ents_sz < sizeof (gpt_partition_entry))
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT entry size");
      goto out;
    }

  hdr->ents = loom_le32toh (hdr->ents);

  if (loom_mul (hdr->ents, hdr->ents_sz, &size))
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME, "GPT table entries overflow");
      goto out;
    }

  hdr->ents_loc = loom_le64toh (hdr->ents_loc);

  if (loom_mul (parent->block_size, hdr->ents_loc, &loc))
    {
      loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME,
                      "GPT table entries out of range");
      goto out;
    }

  ents = loom_malloc (size);

  if (ents == NULL)
    goto out;

  if ((error = loom_block_dev_read (parent, loc, size, ents)))
    {
      loom_error_np (error);
      goto out;
    }

  // TODO: validate entries CRC32

  for (usize i = 0; i < hdr->ents; ++i)
    {
      loom_partition partition;
      gpt_partition_entry *ent
          = (gpt_partition_entry *) (ents + i * hdr->ents_sz);
      usize offset, blocks;

      if (!loom_memcmp (ent->partition_type, gpt_part_type_unused,
                        GPT_GUID_SIZE))
        continue;

      ent->range.start = loom_le64toh (ent->range.start);
      ent->range.end = loom_le64toh (ent->range.end);

      if (ent->range.start > ent->range.end)
        {
          loom_fmt_error (LOOM_ERR_BAD_PART_SCHEME,
                          "bad GPT partition entry range");
          goto out;
        }

      if (loom_checked_cast (ent->range.start, &offset)
          || loom_add (ent->range.end - offset, 1, &blocks))
        // FIXME: Emit warning for out of range partition.
        continue;

      loom_partition_init (&partition, parent, offset, blocks);

      if ((ret_val = hook (parent, &partition, ctx)))
        {
          loom_error_np (LOOM_ERR_HOOK);
          goto out;
        }
    }

  ret_val = 0;
out:
  loom_free (ents);
  loom_free (hdr);

  return ret_val;
}

LOOM_MOD (gpt)

LOOM_MOD_INIT () { loom_partition_scheme_register (&gpt_partition_scheme); }

LOOM_MOD_DEINIT ()
{
  loom_partition_scheme_unregister (&gpt_partition_scheme);
}