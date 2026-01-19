#include <asm-generic/errno-base.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "loom/module.h"

typedef struct
{
  size_t len, cap;
  char *data;
} modules_t;

static int binfd = -1, newbinfd = -1, modfd = -1;
static modules_t mods;

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

static void
read_all (int _fd, char *data, size_t len)
{
  ssize_t slen, rlen;

  if (len > SSIZE_MAX)
    {
      errno = EOVERFLOW;
      error ();
    }

  slen = (ssize_t) len;

  while (slen)
    {
      rlen = read (_fd, data, (size_t) slen);

      if (rlen <= 0)
        error ();
      else if (rlen > slen)
        error_with ("Read %zu more bytes than available?", rlen - slen);

      slen -= rlen;
      data += rlen;
    }
}

static void
write_all (int _fd, const char *data, size_t len)
{
  ssize_t slen, wlen;

  if (len > SSIZE_MAX)
    {
      errno = EOVERFLOW;
      error ();
    }

  slen = (ssize_t) len;

  while (slen)
    {
      wlen = write (_fd, data, (size_t) slen);

      if (wlen <= 0)
        error ();
      else if (wlen > slen)
        error_with ("Wrote %zu more bytes than available?", wlen - slen);

      slen -= wlen;
      data += wlen;
    }
}

int
main (int argc, char *argv[])
{
  loom_module_header_t hdr;
  struct stat buf;
  uint32_t table_size, *table;
  size_t binsize, modindex = 0, table_bytes;
  char *bindata;

  const char *inputpath = NULL, *outputpath = NULL;

  size_t nargmods = 0, cargmods = 0;
  char **argmods = NULL;

  for (int i = 1; i < argc; ++i)
    {
      if (!strncmp (argv[i], "-i", 3))
        {
          if (i == argc - 1)
            error_with ("Expected binary name after -i");
          inputpath = argv[i + 1];
          ++i;
          continue;
        }

      if (!strncmp (argv[i], "-o", 3))
        {
          if (i == argc - 1)
            error_with ("Expected binary name after -o");
          outputpath = argv[i + 1];
          ++i;
          continue;
        }

      if (cargmods <= nargmods)
        {
          if (!cargmods)
            cargmods = 1;
          while (cargmods < nargmods)
            cargmods *= 2;

          argmods = realloc (argmods, sizeof (char *) * cargmods);
          if (!argmods)
            error ();
        }

      argmods[nargmods++] = argv[i];
    }

  if (inputpath == NULL)
    error_with ("Expected input binary (specify one with -i <binary>)");

  if (outputpath == NULL)
    error_with ("Expected output name (specify one with -o <name>)");

  binfd = open (inputpath, O_RDONLY);
  if (binfd < 0)
    error ();

  if (fstat (binfd, &buf))
    error ();

  if (!buf.st_size)
    error_with ("Input binary '%s' is empty", inputpath);
  else if (buf.st_size < 0)
    error_with ("Invalid input binary size");

  binsize = (size_t) buf.st_size;

  newbinfd = open (outputpath, O_WRONLY | O_TRUNC | O_APPEND | O_CREAT,
                   S_IRUSR | S_IWUSR);
  if (newbinfd < 0)
    error ();

  table_size = nargmods;
  table_bytes = (size_t) (table_size + 1) * sizeof (*table);

  // Add one entry for the null terminator.
  table = malloc (table_bytes);

  if (!table)
    error ();

  // Zero null terminator.
  table[table_size] = 0;

  for (unsigned int i = 0; i < nargmods; ++i)
    {
      const char *modpath = argmods[i];
      modfd = open (modpath, O_RDONLY);
      size_t modlen, reqlen;

      if (modfd < 0 || fstat (modfd, &buf))
        error ();
      if (!buf.st_size)
        error_with ("Input module '%s' is empty", modpath);

      modlen = (size_t) buf.st_size;
      reqlen = mods.len + (size_t) buf.st_size;

      if (reqlen > mods.cap)
        {
          char *newdata;

          if (!mods.cap)
            mods.cap = 1;
          while (mods.cap < reqlen)
            mods.cap *= 2;

          newdata = realloc (mods.data, mods.cap);
          if (!newdata)
            error ();

          mods.data = newdata;
        }

      if (modindex >= table_size)
        error_with ("Module table ran out of entries?");

      read_all (modfd, mods.data + mods.len, modlen);

      mods.len += modlen;
      table[modindex++] = htole32 (modlen);

      close (modfd);
      modfd = -1;
    }

  hdr.magic = htole32 (LOOM_MODULE_HEADER_MAGIC);
  hdr.taboff = htole32 (sizeof (hdr));
  hdr.modoff = htole32 (sizeof (hdr) + table_bytes);
  hdr.size = htole32 (sizeof (hdr) + table_bytes + mods.len);

  bindata = malloc (binsize);
  if (!bindata)
    error ();

  // Read input binary.
  read_all (binfd, bindata, binsize);

  // Write input binary.
  write_all (newbinfd, bindata, binsize);

  // Write the header.
  write_all (newbinfd, (char *) &hdr, sizeof (hdr));

  // Write the module length table.
  write_all (newbinfd, (char *) table, table_bytes);

  // Write the module ELF files.
  write_all (newbinfd, mods.data, mods.len);

  {
    size_t align;
    if (fstat (newbinfd, &buf))
      error ();
    align = ((size_t) buf.st_size) & 511;
    if (align)
      {
        char tmp[512] = { 0 };
        align = 512 - align;
        write_all (newbinfd, tmp, align);
      }
  }

  if (fsync (newbinfd))
    error ();

  return 0;
}