#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "osdep.h"

#define LOOM_NOSTDINT
#include "loom/error.h"

loom_error
loomFileOpen (const char *path, uint oflags, loom_file *file)
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

loom_error
loomFileGetMeta (loom_file file, loom_file_meta *meta)
{
  struct stat buf;

  if (meta == NULL)
    return LOOM_ERR_BAD_ARG;

  if (fstat (file, &buf) < 0 || buf.st_size < 0)
    return LOOM_ERR_PLATFORM;

  meta->size = (usize) buf.st_size;

  return LOOM_ERR_NONE;
}

loom_error
loomFileSync (loom_file file)
{
  if (fsync (file) < 0)
    return LOOM_ERR_PLATFORM;
  return LOOM_ERR_NONE;
}

loom_error
loomFileClose (loom_file file)
{
  if (close (file) < 0)
    return LOOM_ERR_PLATFORM;
  return LOOM_ERR_NONE;
}

loom_error
loomFileRead (loom_file file, void *buf, usize nbytes)
{
  while (nbytes)
    {
      ssize_t nread = read (file, (char *) buf, nbytes);

      if (nread < 0 && errno == EINTR)
        continue;

      if (nread < 0 || (usize) nread > nbytes)
        return LOOM_ERR_PLATFORM;

      if (!nread && nbytes)
        return LOOM_ERR_PLATFORM;

      buf = (char *) buf + nread;
      nbytes -= (usize) nread;
    }

  return LOOM_ERR_NONE;
}

loom_error
loomFileReadAt (loom_file file, void *buf, usize offset, usize nbytes)
{
  while (nbytes)
    {
      ssize_t read
          = pread (file, (char *) buf + offset, nbytes, (__off_t) offset);

      if (read < 0 && errno == EINTR)
        continue;

      if (read <= 0 || (usize) read > nbytes)
        return LOOM_ERR_PLATFORM;

      offset += (usize) read;
      nbytes -= (usize) read;
    }

  return LOOM_ERR_NONE;
}

loom_error
loomFileReadAll (loom_file file, loom_slice_t *slice)
{
  struct stat buf;
  usize size;
  __off_t off = 0;

  if (slice == NULL)
    return LOOM_ERR_BAD_ARG;

  slice->size = 0;
  slice->buf = NULL;

  if (fstat (file, &buf) || buf.st_size < 0)
    return LOOM_ERR_PLATFORM;

  if (!buf.st_size)
    return LOOM_ERR_NONE;

  size = (usize) buf.st_size;

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

      if (read < 0 || (usize) read > size)
        goto fail;

      if (!read && size)
        goto fail;

      off += read;
      size -= (usize) read;
    }

  return LOOM_ERR_NONE;

fail:
  free (slice->buf);
  slice->size = 0;
  slice->buf = NULL;
  return LOOM_ERR_PLATFORM;
}

loom_error
loomFileWrite (loom_file file, void *buf, usize nbytes)
{
  while (nbytes)
    {
      ssize_t nwrite = write (file, (char *) buf, nbytes);

      if (nwrite < 0 && errno == EINTR)
        continue;

      if (nwrite < 0 || (usize) nwrite > nbytes)
        return LOOM_ERR_PLATFORM;

      if (!nwrite && nbytes)
        return LOOM_ERR_PLATFORM;

      buf = (char *) buf + nwrite;
      nbytes -= (usize) nwrite;
    }

  return LOOM_ERR_NONE;
}

loom_error
loomFileWriteAt (loom_file file, void *buf, usize offset, usize nbytes)
{
  while (nbytes)
    {
      ssize_t write = pwrite (file, (char *) buf, nbytes, (__off_t) offset);

      if (write < 0 && errno == EINTR)
        continue;

      if (write <= 0 || (usize) write > nbytes)
        return LOOM_ERR_PLATFORM;

      usize uwrite = (usize) write;
      buf = (char *) buf + uwrite;
      offset += uwrite;
      nbytes -= uwrite;
    }

  return LOOM_ERR_NONE;
}

const char *
loomOsError ()
{
  return strerror (errno);
}