#include <endian.h>
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

static loom_file bin, kernel, initrd, new_bin, mod;
static modules mods;

static void
error ()
{
  fprintf (stderr, "Failed to write module table: %s\n", strerror (errno));
  exit (1);
}

static void
error_with (const char *fmt, ...)
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

  usize mod_index = 0, table_bytes, table_size, bin_size, kernel_size = 0,
        initrd_size = 0;

  u32 *table;
  char *bin_data, *kernel_data, *initrd_data;

  const char *in_path = NULL, *kernel_path = NULL, *initrd_path = NULL,
             *out_path = NULL;

  module_args mod_args = { 0 };

  for (int i = 1; i < argc; ++i)
    {
      if (!strncmp (argv[i], "-i", 3))
        {
          if (i == argc - 1)
            error_with ("Expected binary name after -i");
          in_path = argv[i + 1];
          ++i;
          continue;
        }

      if (!strncmp (argv[i], "-o", 3))
        {
          if (i == argc - 1)
            error_with ("Expected binary name after -o");
          out_path = argv[i + 1];
          ++i;
          continue;
        }

      if (!strncmp (argv[i], "-k", 3))
        {
          if (i == argc - 1)
            error_with ("Expected kernel name after -k");
          kernel_path = argv[i + 1];
          ++i;
          continue;
        }

      if (!strncmp (argv[i], "-d", 3))
        {
          if (i == argc - 1)
            error_with ("Expected initrd name after -d");
          initrd_path = argv[i + 1];
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
    error_with ("Expected input binary (specify one with -i <binary>)");

  if (kernel_path)
    {
      if (loom_file_open (kernel_path, LOOM_O_RDONLY, &kernel))
        error ();

      if (loom_file_get_meta (kernel, &file_meta))
        error ();

      kernel_size = file_meta.size;
    }

  if (initrd_path)
    {
      if (loom_file_open (initrd_path, LOOM_O_RDONLY, &initrd))
        error ();

      if (loom_file_get_meta (initrd, &file_meta))
        error ();

      initrd_size = file_meta.size;
    }

  if (out_path == NULL)
    error_with ("Expected output name (specify one with -o <name>)");

  if (loom_file_open (in_path, LOOM_O_RDONLY, &bin))
    error ();

  if (loom_file_get_meta (bin, &file_meta))
    error ();

  if (!file_meta.size)
    error_with ("Input binary '%s' is empty", in_path);

  bin_size = file_meta.size;

  if (loom_file_open (out_path,
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

      if (loom_file_open (mod_path, LOOM_O_RDONLY, &mod))
        error ();

      if (loom_file_get_meta (mod, &file_meta))
        error ();

      mod_size = file_meta.size;

      if (!mod_size)
        error_with ("Input module '%s' is empty", mod_path);

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
        error_with ("Module table ran out of entries?");

      if (loom_file_read (mod, mods.data + mods.size, mod_size))
        error ();

      if (mod_size > UINT32_MAX)
        error_with ("Module size %lu exceeds 32-bit address space.",
                    (unsigned long) mod_size);

      mods.size += mod_size;
      table[mod_index++] = htole32 ((uint32_t) mod_size);

      if (loom_file_close (mod))
        error ();
    }

  hdr.magic = htole32 (LOOM_MODULE_HEADER_MAGIC);
  hdr.taboff = htole32 (sizeof (hdr));
  hdr.modoff = htole32 (sizeof (hdr) + (uint32_t) table_bytes);
  hdr.size = htole32 ((uint32_t) sizeof (hdr) + (uint32_t) table_bytes
                      + (uint32_t) mods.size);
  hdr.kernel_size = htole32 ((uint32_t) kernel_size);
  hdr.initrdsize = htole32 ((uint32_t) initrd_size);

  bin_data = malloc (bin_size);
  if (bin_data == NULL)
    error ();

  // Read input binary and copy to new binary.
  if (loom_file_read (bin, bin_data, bin_size)
      || loom_file_write (new_bin, bin_data, bin_size))
    error ();

  // Write the header to new binary.
  if (loom_file_write (new_bin, &hdr, sizeof (hdr)))
    error ();

  // Write the module length table.
  if (loom_file_write (new_bin, table, table_bytes))
    error ();

  if (loom_file_write (new_bin, mods.data, mods.size))
    error ();

  // Write the kernel.
  if (kernel_size)
    {
      kernel_data = malloc (kernel_size);
      if (kernel_data == NULL)
        error ();
      if (loom_file_read (kernel, kernel_data, kernel_size))
        error ();
      if (loom_file_write (new_bin, kernel_data, kernel_size))
        error ();
    }

  // Write the initrd.
  if (initrd_size)
    {
      initrd_data = malloc (initrd_size);
      if (initrd_data == NULL)
        error ();
      if (loom_file_read (initrd, initrd_data, initrd_size))
        error ();
      if (loom_file_write (new_bin, initrd_data, initrd_size))
        error ();
    }

  if (loom_file_get_meta (new_bin, &file_meta))
    error ();

  {
    usize align = file_meta.size & 511;
    if (align)
      {
        char tmp[512] = { 0 };
        if (loom_file_write (new_bin, tmp, 512 - align))
          error ();
      }
  }

  if (loom_file_sync (new_bin))
    error ();

  return 0;
}