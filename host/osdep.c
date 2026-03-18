#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "osdep.h"

#define LOOM_NOSTDINT
#include "loom/error.h"

loom_error_t
loom_file_open (const char *path, uint oflags, loom_file_t *file)
{
  int flags = 0;
  uint access;

  if (path == NULL || file == NULL)
    return LOOM_ERR_BAD_ARG;

  access = oflags & (LOOM_O_RDONLY | LOOM_O_WRONLY | LOOM_O_RDWR);

  if (!access || access == LOOM_O_RDONLY)
    flags |= O_RDONLY;
  else if (access == LOOM_O_WRONLY)
    flags |= O_WRONLY;
  else if (access == LOOM_O_RDWR)
    flags |= O_RDWR;
  else
    return LOOM_ERR_BAD_ARG;

  if (oflags & LOOM_O_APPEND)
    flags |= O_APPEND;
  if (oflags & LOOM_O_TRUNC)
    flags |= O_TRUNC;
  if (oflags & LOOM_O_CREAT)
    flags |= O_CREAT;

  int fd;
  if (flags & O_CREAT)
    fd = open (path, flags, 0644);
  else
    fd = open (path, flags);

  if (fd < 0)
    return LOOM_ERR_PLATFORM;

  *file = fd;

  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_meta (loom_file_t file, loom_file_meta_t *meta)
{
  struct stat buf;

  if (meta == NULL)
    return LOOM_ERR_BAD_ARG;

  if (fstat (file, &buf) < 0 || buf.st_size < 0)
    return LOOM_ERR_PLATFORM;

  meta->size = (loom_usize_t) buf.st_size;

  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_sync (loom_file_t file)
{
  if (fsync (file) < 0)
    return LOOM_ERR_PLATFORM;
  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_close (loom_file_t file)
{
  if (close (file) < 0)
    return LOOM_ERR_PLATFORM;
  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_read (loom_file_t file, void *buf, loom_usize_t nbytes)
{
  while (nbytes)
    {
      ssize_t nread = read (file, (char *) buf, nbytes);

      if (nread < 0 && errno == EINTR)
        continue;

      if (nread < 0 || (loom_usize_t) nread > nbytes)
        return LOOM_ERR_PLATFORM;

      if (!nread && nbytes)
        return LOOM_ERR_PLATFORM;

      buf = (char *) buf + nread;
      nbytes -= (loom_usize_t) nread;
    }

  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_read_all (loom_file_t file, loom_slice_t *slice)
{
  struct stat buf;
  loom_usize_t size;
  __off_t off = 0;

  if (slice == NULL)
    return LOOM_ERR_BAD_ARG;

  slice->size = 0;
  slice->buf = NULL;

  if (fstat (file, &buf) || buf.st_size < 0)
    return LOOM_ERR_PLATFORM;

  if (!buf.st_size)
    return LOOM_ERR_NONE;

  size = (loom_usize_t) buf.st_size;

  slice->buf = malloc (size + 1);
  if (slice->buf == NULL)
    return LOOM_ERR_ALLOC;

  slice->size = size;
  ((char *) slice->buf)[size] = '\0';

  while (size)
    {
      ssize_t read = pread (file, (char *) slice->buf + off, size, off);

      if (read < 0 && errno == EINTR)
        continue;

      if (read < 0 || (loom_usize_t) read > size)
        goto fail;

      if (!read && size)
        goto fail;

      off += read;
      size -= (loom_usize_t) read;
    }

  return LOOM_ERR_NONE;

fail:
  free (slice->buf);
  slice->size = 0;
  slice->buf = NULL;
  return LOOM_ERR_PLATFORM;
}

loom_error_t
loom_file_write (loom_file_t file, void *buf, loom_usize_t nbytes)
{
  while (nbytes)
    {
      ssize_t nwrite = write (file, (char *) buf, nbytes);

      if (nwrite < 0 && errno == EINTR)
        continue;

      if (nwrite < 0 || (loom_usize_t) nwrite > nbytes)
        return LOOM_ERR_PLATFORM;

      if (!nwrite && nbytes)
        return LOOM_ERR_PLATFORM;

      buf = (char *) buf + nwrite;
      nbytes -= (loom_usize_t) nwrite;
    }

  return LOOM_ERR_NONE;
}

loom_error_t
loom_file_write_exact (loom_file_t file, void *buf, loom_usize_t off,
                       loom_usize_t nbytes)
{
  while (nbytes)
    {
      ssize_t write = pwrite (file, (char *) buf + off, nbytes, (__off_t) off);

      if (write < 0 && errno == EINTR)
        continue;

      if (write <= 0 || (loom_usize_t) write > nbytes)
        return LOOM_ERR_PLATFORM;

      off += (loom_usize_t) write;
      nbytes -= (loom_usize_t) write;
    }

  return LOOM_ERR_NONE;
}