#include "fat.h"
#include "loom/mm.h"
#include "loom/string.h"

int
fatAllocSFN (char **oname, fat_dir_entry *d_entry)
{
  usize size = 0, base_size, ext_size;
  uchar buf[16] = { 0 };

  auto dfname = d_entry->file_name;

  for (base_size = 8; base_size > 0; base_size--)
    if (dfname[base_size - 1] != 0x20)
      break;

  if (dfname[10] != 0x20)
    ext_size = 3;
  else if (dfname[9] != 0x20)
    ext_size = 2;
  else if (dfname[8] != 0x20)
    ext_size = 1;
  else
    ext_size = 0;

  loomMemCopy (buf, dfname, base_size);
  size = base_size;

  if (buf[0] == 0x05)
    buf[0] = 0xE5;

  if (ext_size)
    {
      buf[size++] = '.';
      loomMemCopy (buf + size, dfname + 8, ext_size);
      size += ext_size;
    }

  buf[size++] = '\0';

  if ((*oname = loomAlloc (size)) == null)
    return -1;

  loomMemCopy (*oname, buf, size);

  return 0;
}