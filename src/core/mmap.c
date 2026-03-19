#include "loom/mmap.h"
#include "loom/error.h"
#include "loom/mm.h"
#include "loom/platform.h"

loom_memory_map loom_mmap;

static void
mmap_count_hook (LOOM_UNUSED u64 addr, LOOM_UNUSED u64 length,
                 LOOM_UNUSED loom_memory_type type, void *data)
{
  usize *count = data;
  (*count)++;
}

static void
mmap_fill_hook (u64 addr, u64 length, loom_memory_type type,
                LOOM_UNUSED void *data)
{
  usize count = *(usize *) data;

  if (loom_mmap.count >= count)
    loom_panic ("mmap_iterate_fill");

  loom_mmap.entries[loom_mmap.count++]
      = (loom_mmap_entry) { .addr = addr, .length = length, .type = type };
}

void
loom_mmap_init (void)
{
  usize count = 0;
  loom_platform_mmap_iterate (mmap_count_hook, &count);

  if (!count)
    return;

  loom_mmap.entries = loom_malloc (sizeof (*loom_mmap.entries) * count);
  if (!loom_mmap.entries)
    return;

  loom_platform_mmap_iterate (mmap_fill_hook, &count);
}

int
loom_mmap_iterate (int (*hook) (loom_mmap_entry *entry, void *data),
                   void *data)
{
  int ret_val = 0;

  for (usize i = 0; i < loom_mmap.count; ++i)
    if ((ret_val = hook (loom_mmap.entries + i, data)))
      return ret_val;

  return ret_val;
}