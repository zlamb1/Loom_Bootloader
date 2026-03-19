#include "loom/mmap.h"
#include "loom/error.h"
#include "loom/mm.h"
#include "loom/platform.h"

loom_memory_map loom_mmap;

static void
mmapCountHook (unused u64 addr, unused u64 length,
               unused loom_memory_type type, void *data)
{
  usize *count = data;
  (*count)++;
}

static void
mmapFillHook (u64 addr, u64 length, loom_memory_type type, unused void *data)
{
  usize count = *(usize *) data;

  if (loom_mmap.count >= count)
    loomPanic ("mmapFillHook");

  loom_mmap.entries[loom_mmap.count++]
      = (loom_mmap_entry) { .addr = addr, .length = length, .type = type };
}

void
loomMmapInit (void)
{
  usize count = 0;
  loomPlatformMmapIterate (mmapCountHook, &count);

  if (!count)
    return;

  loom_mmap.entries = loomAlloc (sizeof (*loom_mmap.entries) * count);
  if (!loom_mmap.entries)
    return;

  loomPlatformMmapIterate (mmapFillHook, &count);
}

int
loomMmapIterate (int (*hook) (loom_mmap_entry *entry, void *data), void *data)
{
  int ret_val = 0;

  for (usize i = 0; i < loom_mmap.count; ++i)
    if ((ret_val = hook (loom_mmap.entries + i, data)))
      return ret_val;

  return ret_val;
}