#include "loom/block_dev.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/partition.h"
#include "loom/partition_scheme.h"
#include "loom/string.h"

int gpt_partition_scheme_iterate (loom_partition_scheme_t *,
                                  loom_block_dev_t *,
                                  loom_partition_scheme_hook_t, void *);

static loom_partition_scheme_t gpt_partition_scheme = {
  .iterate = gpt_partition_scheme_iterate,
};

typedef loom_uint64_t lba_t;

typedef struct
{
  lba_t start;
  lba_t end;
} LOOM_PACKED range_t;

typedef struct
{
#define GPT_SIGNATURE "EFI PART"
#define GPT_GUID_SIZE 16
  loom_uint64_t signature;
  loom_uint32_t revision;
  loom_uint32_t hdr_sz;
  loom_uint32_t hdr_crc32;
  loom_uint32_t res1;
  lba_t hdr_loc;
  lba_t backup_loc;
  range_t alloc;
  char disk_uuid[GPT_GUID_SIZE];
  lba_t ents_loc;
  loom_uint32_t ents;
  loom_uint32_t ents_sz;
  loom_uint32_t ents_crc32;
} LOOM_PACKED gpt_header_t;

static char gpt_part_type_unused[GPT_GUID_SIZE] = { 0 };

typedef struct
{
  char partition_type[GPT_GUID_SIZE];
  char partition_uuid[GPT_GUID_SIZE];
  range_t range;
#define GPT_PART_PLATFORM_REQ  (1 << 0)
#define GPT_PART_EFI_IGNORE    (1 << 1)
#define GPT_PART_BIOS_BOOTABLE (1 << 2)
  loom_uint64_t attributes;
  char name[72]; // UTF-16LE
} LOOM_PACKED gpt_partition_entry_t;

int
gpt_partition_scheme_iterate (loom_partition_scheme_t *partition_scheme,
                              loom_block_dev_t *parent,
                              loom_partition_scheme_hook_t hook, void *ctx)
{
  int retval = -1;
  gpt_header_t *hdr = NULL;
  char *ents = NULL;
  loom_error_t error;
  loom_usize_t size, loc;

  (void) partition_scheme;

  hdr = loom_malloc (sizeof (*hdr));
  if (hdr == NULL)
    goto out;

  if ((error = loom_block_dev_read (parent, parent->blocksz, sizeof (*hdr),
                                    (char *) hdr)))
    {
      loom_error_np (error);
      goto out;
    }

  if (loom_memcmp (&hdr->signature, GPT_SIGNATURE, 8))
    {
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT signature");
      goto out;
    }

  hdr->hdr_sz = loom_le32toh (hdr->hdr_sz);

  if (hdr->hdr_sz < sizeof (*hdr))
    {
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT header size");
      goto out;
    }

  // FIXME: validate header CRC32

  hdr->ents_sz = loom_le32toh (hdr->ents_sz);

  if (hdr->ents_sz < sizeof (gpt_partition_entry_t))
    {
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "bad GPT entry size");
      goto out;
    }

  hdr->ents = loom_le32toh (hdr->ents);

  if (loom_mul (hdr->ents, hdr->ents_sz, &size))
    {
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "GPT table entries overflow");
      goto out;
    }

  hdr->ents_loc = loom_le64toh (hdr->ents_loc);

  if (loom_mul (parent->blocksz, hdr->ents_loc, &loc))
    {
      loom_error (LOOM_ERR_BAD_PART_SCHEME, "GPT table entries out of range");
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

  for (loom_usize_t i = 0; i < hdr->ents; ++i)
    {
      loom_partition_t partition;
      gpt_partition_entry_t *ent
          = (gpt_partition_entry_t *) (ents + i * hdr->ents_sz);
      loom_usize_t offset, blocks;

      if (!loom_memcmp (ent->partition_type, gpt_part_type_unused,
                        GPT_GUID_SIZE))
        continue;

      ent->range.start = loom_le64toh (ent->range.start);
      ent->range.end = loom_le64toh (ent->range.end);

      if (ent->range.start > ent->range.end)
        {
          loom_error (LOOM_ERR_BAD_PART_SCHEME,
                      "bad GPT partition entry range");
          goto out;
        }

      if (loom_checked_cast (ent->range.start, &offset)
          || loom_add (ent->range.end - offset, 1, &blocks))
        // FIXME: Emit warning for out of range partition.
        continue;

      loom_partition_init (&partition, parent, offset, blocks);

      if ((retval = hook (parent, &partition, ctx)))
        {
          loom_error_np (LOOM_ERR_HOOK);
          goto out;
        }
    }

  retval = 0;
out:
  loom_free (ents);
  loom_free (hdr);

  return retval;
}

LOOM_MOD (gpt)

LOOM_MOD_INIT () { loom_partition_scheme_register (&gpt_partition_scheme); }

LOOM_MOD_DEINIT ()
{
  loom_partition_scheme_unregister (&gpt_partition_scheme);
}