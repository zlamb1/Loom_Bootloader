#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osdep.h"

#include "loom/module.h"

typedef struct
{
  usize count, cap;
  char **buf;
} module_args;

typedef struct
{
  usize size, cap;
  char *data;
} modules;

static loom_file bin, new_bin, mod;
static modules mods;

static void
error ()
{
  fprintf (stderr, "Failed to write module table: %s\n", strerror (errno));
  exit (1);
}

static void
errorWith (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "Failed to write module table: ");
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
  exit (1);
}

int
main (int argc, char *argv[])
{
  loom_module_header hdr;
  loom_file_meta file_meta;

  usize mod_index = 0, table_bytes, table_size, bin_size;

  u32 *table;
  byte *bin_data;

  const char *in_path = NULL, *out_path = NULL;

  module_args mod_args = { 0 };

  for (int i = 1; i < argc; ++i)
    {
      if (!strncmp (argv[i], "-i", 3))
        {
          if (i == argc - 1)
            errorWith ("Expected binary name after -i");
          in_path = argv[i + 1];
          ++i;
          continue;
        }

      if (!strncmp (argv[i], "-o", 3))
        {
          if (i == argc - 1)
            errorWith ("Expected binary name after -o");
          out_path = argv[i + 1];
          ++i;
          continue;
        }

      if (mod_args.count >= mod_args.cap)
        {
          if (!mod_args.cap)
            mod_args.cap = 1;
          while (mod_args.count >= mod_args.cap)
            mod_args.cap *= 2;

          mod_args.buf
              = realloc (mod_args.buf, sizeof (char *) * mod_args.cap);
          if (!mod_args.buf)
            error ();
        }

      mod_args.buf[mod_args.count++] = argv[i];
    }

  if (in_path == NULL)
    errorWith ("Expected input binary (specify one with -i <binary>)");

  if (out_path == NULL)
    errorWith ("Expected output name (specify one with -o <name>)");

  if (loomFileOpen (in_path, LOOM_O_RDONLY, &bin))
    error ();

  if (loomFileGetMeta (bin, &file_meta))
    error ();

  if (!file_meta.size)
    errorWith ("Input binary '%s' is empty", in_path);

  bin_size = file_meta.size;

  if (loomFileOpen (out_path,
                    LOOM_O_WRONLY | LOOM_O_TRUNC | LOOM_O_APPEND
                        | LOOM_O_CREAT,
                    &new_bin))
    error ();

  table_size = mod_args.count;
  table_bytes = (size_t) (table_size + 1) * sizeof (*table);

  // Add one entry for the null terminator.
  table = malloc (table_bytes);

  if (!table)
    error ();

  // Zero null terminator.
  table[table_size] = 0;

  for (unsigned int i = 0; i < mod_args.count; ++i)
    {
      const char *mod_path = mod_args.buf[i];
      usize mod_size, req_size;

      if (loomFileOpen (mod_path, LOOM_O_RDONLY, &mod))
        error ();

      if (loomFileGetMeta (mod, &file_meta))
        error ();

      mod_size = file_meta.size;

      if (!mod_size)
        errorWith ("Input module '%s' is empty", mod_path);

      req_size = mods.size + mod_size;

      if (req_size > mods.cap)
        {
          char *new_data;

          if (!mods.cap)
            mods.cap = 1;
          while (mods.cap < req_size)
            mods.cap *= 2;

          new_data = realloc (mods.data, mods.cap);
          if (new_data == NULL)
            error ();

          mods.data = new_data;
        }

      if (mod_index >= table_size)
        errorWith ("Module table ran out of entries?");

      if (loomFileRead (mod, mods.data + mods.size, mod_size))
        error ();

      if (mod_size > UINT32_MAX)
        errorWith ("Module size %lu exceeds 32-bit address space.",
                   (unsigned long) mod_size);

      mods.size += mod_size;
      table[mod_index++] = htole32 ((uint32_t) mod_size);

      if (loomFileClose (mod))
        error ();
    }

  loomEndianStore (hdr.magic, LOOM_MODULE_HEADER_MAGIC);
  loomEndianStore (hdr.taboff, sizeof (hdr));
  loomEndianStore (hdr.modoff, sizeof (hdr) + (u32) table_bytes);
  loomEndianStore (hdr.size,
                   (u32) sizeof (hdr) + (u32) table_bytes + (u32) mods.size);

  bin_data = malloc (bin_size);
  if (bin_data == NULL)
    error ();

  usize append_size = (usize) sizeof (hdr) + table_bytes + mods.size;
  append_size = (append_size + 511) / 512 * 512;

  // Read input binary.
  if (loomFileRead (bin, bin_data, bin_size))
    error ();

  // Write load sectors.
  u32 load_sectors = append_size / 512;
  bin_data[416] = (byte) load_sectors;
  bin_data[417] = (byte) (load_sectors >> 8);
  bin_data[418] = (byte) (load_sectors >> 16);
  bin_data[419] = (byte) (load_sectors >> 24);

  // Copy the to the new binary.
  if (loomFileWrite (new_bin, bin_data, bin_size))
    error ();

  // Write the header to new binary.
  if (loomFileWrite (new_bin, &hdr, sizeof (hdr)))
    error ();

  // Write the module length table.
  if (loomFileWrite (new_bin, table, table_bytes))
    error ();

  if (loomFileWrite (new_bin, mods.data, mods.size))
    error ();

  if (loomFileGetMeta (new_bin, &file_meta))
    error ();

  {
    usize align = file_meta.size & 511;
    if (align)
      {
        char tmp[512] = { 0 };
        if (loomFileWrite (new_bin, tmp, 512 - align))
          error ();
      }
  }

  if (loomFileSync (new_bin))
    error ();

  return 0;
}