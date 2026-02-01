#include "loom/mmap.h"
#include "loom/arch.h"
#include "loom/error.h"
#include "loom/mm.h"

loom_mmap_t loom_mmap;

static void
mmap_count_hook (LOOM_UNUSED loom_uint64_t address,
                 LOOM_UNUSED loom_uint64_t length,
                 LOOM_UNUSED loom_memory_type_t type, void *data)
{
  loom_usize_t *count = data;
  (*count)++;
}

static void
mmap_fill_hook (loom_uint64_t address, loom_uint64_t length,
                loom_memory_type_t type, LOOM_UNUSED void *data)
{
  loom_usize_t count = *(loom_usize_t *) data;

  if (loom_mmap.count >= count)
    loom_panic ("mmap_iterate_fill");

  loom_mmap.entries[loom_mmap.count++] = (loom_mmap_entry_t) {
    .address = address, .length = length, .type = type
  };
}

void
loom_mmap_init (void)
{
  loom_usize_t count = 0;
  loom_arch_mmap_iterate (mmap_count_hook, &count);

  if (!count)
    return;

  loom_mmap.entries = loom_malloc (sizeof (*loom_mmap.entries) * count);
  if (!loom_mmap.entries)
    return;

  loom_arch_mmap_iterate (mmap_fill_hook, &count);
}

int
loom_mmap_iterate (int (*hook) (loom_mmap_entry_t *entry, void *data),
                   void *data)
{
  int retval = 0;

  for (loom_usize_t i = 0; i < loom_mmap.count; ++i)
    if ((retval = hook (loom_mmap.entries + i, data)))
      return retval;

  return retval;
}