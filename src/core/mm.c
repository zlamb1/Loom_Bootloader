#include <stddef.h>

#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/string.h"
#include "loom/types.h"

#define CHUNK_SIZE         8U
#define CHUNK_SIZE_MINUS_1 (CHUNK_SIZE - 1)

#define MIN_ALLOC CHUNK_SIZE * 2

#define ARENA_MAGIC_OF(X)                                                     \
  ({                                                                          \
    arena_t *a = (X);                                                         \
    ARENA_MAGIC ^ a->length ^ (address) a->freelist ^ (address) a->next;      \
  })

#define CHUNK_ALIGN_DOWN(X) ((X) & ~CHUNK_SIZE_MINUS_1)
#define CHUNK_ALIGN_UP(X)   CHUNK_ALIGN_DOWN ((X) + CHUNK_SIZE_MINUS_1)

#define heapPanic() loomPanic ("heap corruption detected")

typedef struct arena_t
{
  usize length;
  struct free_chunk *freelist;
  struct arena_t *next;
#define ARENA_MAGIC 0xFA159B7E
  usize magic;
} arena_t;

typedef struct
{
#define CHUNK_FLAG_INUSE (1U << 0)
#define CHUNK_FLAG_MASK  (7UL)
#define CHUNK_SIZE_MASK  (~7UL)
  usize prev_size;
  usize size;
} chunk_t;

typedef struct free_chunk
{
  chunk_t base;
  struct free_chunk *prev, *next;
} free_chunk_t;

compile_assert (CHUNK_SIZE >= alignof (arena_t),
                "arena_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (MIN_ALLOC >= sizeof (arena_t),
                "arena_t must fit within MIN_ALLOC.");

compile_assert (CHUNK_SIZE >= alignof (chunk_t),
                "chunk_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (CHUNK_SIZE >= sizeof (chunk_t),
                "chunk_t must fit within CHUNK_SIZE.");

compile_assert (
    CHUNK_SIZE >= alignof (free_chunk_t),
    "free_chunk_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (MIN_ALLOC >= sizeof (free_chunk_t),
                "free_chunk_t must fit within MIN_ALLOC.");

static arena_t *arenas = NULL;

void
loomMMAddRegion (usize addr, usize length)
{
  usize tmp;
  address modend;
  bool exit = false;

  if (addr > USIZE_MAX - length)
    length = USIZE_MAX - addr;

  if (addr > USIZE_MAX - CHUNK_SIZE_MINUS_1)
    return;

  tmp = CHUNK_ALIGN_UP (addr);

  if (loomSub (length, tmp - addr, &length))
    return;

  length = CHUNK_ALIGN_DOWN (length);

  addr = tmp;

  modend = loomModEndGet ();

  if (addr < loom_modbase && addr + length > loom_modbase)
    {
      exit = 1;
      loomMMAddRegion (addr, loom_modbase - addr);
    }

  if (addr < modend && addr + length > modend)
    {
      exit = 1;
      loomMMAddRegion (modend, (addr + length) - modend);
    }

  if (exit || length < CHUNK_SIZE * 2 + MIN_ALLOC)
    return;

  free_chunk_t *fchunk = (free_chunk_t *) (addr + MIN_ALLOC);

  *fchunk = (free_chunk_t) {
    .base = { .prev_size = CHUNK_FLAG_INUSE, .size = length - MIN_ALLOC },
    .prev = NULL,
    .next = NULL,
  };

  arena_t *arena = (arena_t *) addr;
  arena->length = length;
  arena->freelist = fchunk;
  arena->next = arenas;
  arena->magic = ARENA_MAGIC_OF (arena);

  arenas = arena;
}

static void
chunkUpdateNext (arena_t *arena, chunk_t *chunk, bool differ)
{
  chunk_t *nchunk;
  usize chunk_size = chunk->size & CHUNK_SIZE_MASK;

  address addr = (address) chunk, arena_end = (address) arena + arena->length;

  // If we either overflow the address space or the arena,
  // something went wrong.
  if (loomAdd (addr, chunk_size, &addr) || addr > arena_end)
    heapPanic ();

  // We are the last chunk. There is no next chunk to update.
  if (addr == arena_end)
    return;

  // Minimum MIN_ALLOC per free_chunk_t or allocation.
  // If we have less than that, something went wrong.
  if (arena_end - addr < MIN_ALLOC)
    heapPanic ();

  // If our address is misaligned, something went wrong.
  if (addr & CHUNK_SIZE_MINUS_1)
    heapPanic ();

  nchunk = (chunk_t *) addr;

  int i1 = chunk->size & CHUNK_FLAG_INUSE,
      i2 = nchunk->prev_size & CHUNK_FLAG_INUSE;

  if ((differ && i1 == i2) || (!differ && i1 != i2))
    heapPanic ();

  nchunk->prev_size = chunk->size;
}

static void
freeChunkLink (arena_t *arena, free_chunk_t *fchunk)
{
  fchunk->prev = NULL;
  fchunk->next = arena->freelist;

  if (arena->freelist)
    {
      if (arena->freelist->prev)
        heapPanic ();

      arena->freelist->prev = fchunk;
    }

  arena->freelist = fchunk;
  arena->magic = ARENA_MAGIC_OF (arena);
}

static void
freeChunkUnlink (arena_t *arena, free_chunk_t *fchunk)
{
  if (fchunk->prev)
    fchunk->prev->next = fchunk->next;
  else
    {
      if (arena->freelist != fchunk)
        heapPanic ();

      arena->freelist = fchunk->next;
      arena->magic = ARENA_MAGIC_OF (arena);
    }

  if (fchunk->next)
    fchunk->next->prev = fchunk->prev;

  fchunk->prev = fchunk->next = NULL;
}

static void *
freeChunkSplit (arena_t *arena, free_chunk_t *fchunk, usize offset, usize size)
{
  chunk_t *new_chunk;
  usize chunk_size, flags;

  if (offset & CHUNK_SIZE_MINUS_1 || (offset && offset < MIN_ALLOC))
    loomPanic ("free_chunk_split: invalid offset %lu", (unsigned long) offset);

  if (!size || size & CHUNK_SIZE_MINUS_1)
    loomPanic ("free_chunk_split: invalid size %lu", (unsigned long) size);

  flags = fchunk->base.size & CHUNK_FLAG_MASK;

  if (flags & CHUNK_FLAG_INUSE)
    loomPanic ("free_chunk_split: invalid chunk with in use flag");

  chunk_size = fchunk->base.size & CHUNK_SIZE_MASK;

  if (offset >= chunk_size)
    loomPanic ("free_chunk_split: offset %lu >= chunk size %lu",
               (unsigned long) offset, (unsigned long) chunk_size);

  if (offset)
    {
      new_chunk = (chunk_t *) assume_aligned ((char *) fchunk + offset,
                                              alignof (chunk_t));

      chunk_size -= offset;

      *new_chunk = (chunk_t) {
        .prev_size = (fchunk->base.size = offset | flags),
      };
    }
  else
    {
      freeChunkUnlink (arena, fchunk);
      new_chunk = &fchunk->base;
    }

  if (chunk_size < size)
    loomPanic ("free_chunk_split: chunk size too small");

  if (chunk_size - size >= MIN_ALLOC)
    {
      free_chunk_t *nfchunk = (free_chunk_t *) assume_aligned (
          (char *) new_chunk + size, alignof (free_chunk_t));

      new_chunk->size = size | flags | CHUNK_FLAG_INUSE;

      *nfchunk
          = (free_chunk_t) { .base = { .prev_size = new_chunk->size,
                                       .size = (chunk_size - size) | flags } };

      freeChunkLink (arena, nfchunk);

      chunkUpdateNext (arena, &nfchunk->base, 0);
    }
  else
    {
      new_chunk->size = chunk_size | flags | CHUNK_FLAG_INUSE;
      chunkUpdateNext (arena, new_chunk, 1);
    }

  return (char *) new_chunk + CHUNK_SIZE;
}

void *
loomAlloc (usize size)
{
  if (loomAdd (size, CHUNK_SIZE_MINUS_1, &size)
      || ((size &= ~CHUNK_SIZE_MINUS_1), loomAdd (size, CHUNK_SIZE, &size)))
    {
      loomErrorFmt (LOOM_ERR_OVERFLOW, "requested allocation size is too big");
      return NULL;
    }

  LOOM_LIST_ITERATE (arenas, arena)
  {
    free_chunk_t *fchunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heapPanic ();

    fchunk = arena->freelist;

    while (fchunk)
      {
        usize chunk_size;

        if (fchunk->base.size & CHUNK_FLAG_INUSE)
          heapPanic ();

        chunk_size = fchunk->base.size & CHUNK_SIZE_MASK;

        if (chunk_size >= size)
          return freeChunkSplit (arena, fchunk, 0, size);

        fchunk = fchunk->next;
      }
  }

  loomError (LOOM_ERR_ALLOC);

  return NULL;
}

void *
loomZeroAlloc (usize size)
{
  void *p = loomAlloc (size);

  if (!p)
    return p;

  loomMemSet (p, 0, size);

  return p;
}

void *
loomArrayAlloc (usize n, usize size)
{
  if (!size)
    return NULL;

  if (loomMul (n, size, &size))
    return NULL;

  return loomZeroAlloc (size);
}

void *
loomRealloc (void *p, usize n)
{
  usize copy, oldsize;
  address addr = (address) p;

  chunk_t *chunk;
  void *new = loomAlloc (n);

  if (!new)
    return NULL;
  else if (!p)
    return new;

  if (addr & CHUNK_SIZE_MINUS_1)
    loomPanic ("realloc: invalid pointer %p", p);

  // This is undefined behavior if p
  // is misaligned or is an invalid pointer.
  chunk = (chunk_t *) assume_aligned ((char *) p - CHUNK_SIZE,
                                      alignof (chunk_t));

  // Note: Some garbage might get copied if the old allocation contained an end
  // chunk that couldn't fit a free chunk. Shouldn't matter.
  oldsize = (chunk->size & CHUNK_SIZE_MASK) - CHUNK_SIZE;
  copy = n > oldsize ? oldsize : n;

  loomMemCopy (new, p, copy);

  // Let free do all the error checking here.
  loomFree (p);

  return new;
}

void *
loomMemAlign (usize size, usize align)
{
  return loomMemAlignRange (size, align, 0, ADDRESS_MAX);
}

void *
loomMemAlignRange (usize size, usize align, address min_addr, address max_addr)
{
  if (loomAdd (size, CHUNK_SIZE_MINUS_1, &size)
      || ((size &= ~CHUNK_SIZE_MINUS_1), loomAdd (size, CHUNK_SIZE, &size)))
    {
      loomErrorFmt (LOOM_ERR_OVERFLOW, "requested allocation size is too big");
      return NULL;
    }

  if (min_addr > max_addr || size > max_addr - min_addr)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG,
                    "memalign range cannot fit allocation of size %lu",
                    (unsigned long) size);
      return NULL;
    }

  if (align & (align - 1))
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "invalid requested alignment %lu",
                    (unsigned long) align);
      return NULL;
    }

  if (align < CHUNK_SIZE)
    align = CHUNK_SIZE;

  LOOM_LIST_ITERATE (arenas, arena)
  {
    free_chunk_t *fchunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heapPanic ();

    fchunk = arena->freelist;

    while (fchunk)
      {
        address start = (address) fchunk, end;
        usize chunk_size = fchunk->base.size & CHUNK_SIZE_MASK;

        if (loomAdd (start, chunk_size, &end)
            || fchunk->base.size & CHUNK_FLAG_INUSE)
          heapPanic ();

        if (start < min_addr)
          start = min_addr;

        start = (start + (align - 1)) & ~(align - 1);

        while (start < USIZE_MAX - size && start + size <= max_addr
               && start < end && end - start >= size)
          {
            usize offset;

            if (loomSub (start, (address) fchunk, &offset)
                || loomSub (offset, CHUNK_SIZE, &offset)
                || offset == CHUNK_SIZE)
              {
                if (loomAdd (start, align, &start))
                  break;
                continue;
              }

            return freeChunkSplit (arena, fchunk, offset, size);
          }

        fchunk = fchunk->next;
      }
  }

  loomError (LOOM_ERR_ALLOC);

  return NULL;
}

void
loomFree (void *p)
{
  address addr = (address) p;

  if (p == NULL)
    return;

  if (loomSub (addr, CHUNK_SIZE, &addr) || addr & CHUNK_SIZE_MINUS_1
      || addr > ADDRESS_MAX - MIN_ALLOC)
    loomPanic ("free: invalid pointer %p", p);

  LOOM_LIST_ITERATE (arenas, arena)
  {
    address arena_start, arena_end, chunk_end;
    chunk_t *chunk;
    free_chunk_t *fchunk;
    bool inuse, differ = true;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heapPanic ();

    arena_start = (address) arena + MIN_ALLOC;
    arena_end = (address) arena + arena->length;

    if (addr < arena_start || addr + MIN_ALLOC > arena_end)
      continue;

    chunk = (chunk_t *) addr;

    // Chunk should be allocated. If not, we got
    // a bad pointer or there is heap corruption.
    if (!(chunk->size & CHUNK_FLAG_INUSE))
      heapPanic ();

    fchunk = (free_chunk_t *) chunk;
    fchunk->base.size &= ~CHUNK_FLAG_INUSE;

    // Speculatively link the free chunk.
    // If we perform any backward coalescing, we unlink this.
    freeChunkLink (arena, fchunk);

    inuse = fchunk->base.prev_size & CHUNK_FLAG_INUSE;

    // Perform backward coalescing.
    while (!inuse)
      {
        free_chunk_t *bchunk;
        usize prev_size = fchunk->base.prev_size & CHUNK_SIZE_MASK, size;

        if (prev_size < MIN_ALLOC || loomSub (addr, prev_size, &addr)
            || addr < arena_start)
          heapPanic ();

        bchunk = (free_chunk_t *) addr;
        if (bchunk->base.size & CHUNK_FLAG_INUSE
            || (bchunk->base.size & CHUNK_SIZE_MASK) != prev_size)
          heapPanic ();

        size = fchunk->base.size & CHUNK_SIZE_MASK;

        freeChunkUnlink (arena, fchunk);

        // Grow back chunk.
        usize flags = bchunk->base.size & CHUNK_FLAG_MASK;
        bchunk->base.size = (prev_size + size) | flags;

        fchunk = bchunk;
        inuse = bchunk->base.prev_size & CHUNK_FLAG_INUSE;
      }

    chunk_end = (address) fchunk + (fchunk->base.size & CHUNK_SIZE_MASK);

    inuse = 0;

    // Coalesce forward.
    while (chunk_end < arena_end)
      {
        free_chunk_t *nchunk;
        usize size = fchunk->base.size & CHUNK_SIZE_MASK;

        chunk = (chunk_t *) chunk_end;
        if (chunk->size & CHUNK_FLAG_INUSE)
          {
            inuse = 1;
            break;
          }

        differ = 0;

        nchunk = (free_chunk_t *) chunk;

        freeChunkUnlink (arena, nchunk);

        usize flags = fchunk->base.size & CHUNK_FLAG_MASK,
              newsize = nchunk->base.size & CHUNK_SIZE_MASK;

        if (newsize < MIN_ALLOC || loomAdd (size, newsize, &size)
            || loomAdd (chunk_end, newsize, &chunk_end))
          heapPanic ();

        fchunk->base.size = size | flags;
      }

    // Either heap corruption or some logic error.
    if (chunk_end != arena_end && !inuse)
      heapPanic ();

    chunkUpdateNext (arena, &fchunk->base, differ);

    // All done.
    return;
  }

  loomPanic ("free: invalid pointer %p", p);
}

int
loomMMIterate (int (*hook) (address p, usize n, bool is_free, void *data),
               void *data)
{
  if (!hook)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "invalid memory manager iterate hook %p",
                    hook);
      return -1;
    }

  // Note: This serves as a way of validating the heap as well as
  // hooking and iterating free chunks and allocations.
  LOOM_LIST_ITERATE (arenas, arena)
  {
    address addr = (address) arena, arena_end;
    usize prev_size;
    bool first = true;
    chunk_t *chunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic
        || loomAdd (addr, arena->length, &arena_end))
      heapPanic ();

    addr += MIN_ALLOC;

    while (addr < arena_end)
      {
        int retval;

        usize chunk_size;
        chunk = (chunk_t *) addr;

        chunk_size = chunk->size & CHUNK_SIZE_MASK;

        if (chunk_size < MIN_ALLOC || (chunk_size & CHUNK_SIZE_MINUS_1)
            || loomAdd (addr, chunk_size, &addr) || addr > arena_end
            || (!first && prev_size != chunk->prev_size))
          heapPanic ();

        retval = hook (addr - chunk_size + CHUNK_SIZE, chunk_size - CHUNK_SIZE,
                       !(chunk->size & CHUNK_FLAG_INUSE), data);

        if (retval)
          return retval;

        first = 0;
        prev_size = chunk->size;
      }

    if (addr != arena_end)
      heapPanic ();
  }

  return 0;
}