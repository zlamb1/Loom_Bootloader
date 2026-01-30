#include <stddef.h>

#include "loom/error.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/string.h"

#define CHUNK_SIZE         8U
#define CHUNK_SIZE_MINUS_1 (CHUNK_SIZE - 1)

#define MIN_ALLOC CHUNK_SIZE * 2

#define ARENA_MAGIC_OF(X)                                                     \
  ({                                                                          \
    arena_t *a = (X);                                                         \
    ARENA_MAGIC ^ a->length ^ (loom_address_t) a->freelist                    \
        ^ (loom_address_t) a->next;                                           \
  })

#define CHUNK_ALIGN_DOWN(X) ((X) & ~CHUNK_SIZE_MINUS_1)
#define CHUNK_ALIGN_UP(X)   CHUNK_ALIGN_DOWN ((X) + CHUNK_SIZE_MINUS_1)

#define heap_panic() loom_panic ("heap corruption detected")

typedef struct arena_t
{
  loom_usize_t length;
  struct free_chunk *freelist;
  struct arena_t *next;
#define ARENA_MAGIC 0xFA159B7E
  loom_usize_t magic;
} arena_t;

typedef struct
{
#define CHUNK_FLAG_INUSE (1U << 0)
#define CHUNK_FLAG_MASK  (~7UL)
  loom_usize_t prev_size;
  loom_usize_t size;
} chunk_t;

typedef struct free_chunk
{
  chunk_t base;
  struct free_chunk *prev, *next;
} free_chunk_t;

compile_assert (CHUNK_SIZE >= loom_alignof (arena_t),
                "arena_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (MIN_ALLOC >= sizeof (arena_t),
                "arena_t must fit within MIN_ALLOC.");

compile_assert (CHUNK_SIZE >= loom_alignof (chunk_t),
                "chunk_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (CHUNK_SIZE >= sizeof (chunk_t),
                "chunk_t must fit within CHUNK_SIZE.");

compile_assert (
    CHUNK_SIZE >= loom_alignof (free_chunk_t),
    "free_chunk_t alignment must be less than or equal to CHUNK_SIZE.");

compile_assert (MIN_ALLOC >= sizeof (free_chunk_t),
                "free_chunk_t must fit within MIN_ALLOC.");

static arena_t *arenas = NULL;

void
loom_mm_add_region (loom_usize_t address, loom_usize_t length)
{
  loom_usize_t tmp;
  loom_address_t modend;
  loom_bool_t exit = 0;

  if (address > LOOM_USIZE_MAX - length)
    length = LOOM_USIZE_MAX - address;

  if (address > LOOM_USIZE_MAX - CHUNK_SIZE_MINUS_1)
    return;

  tmp = CHUNK_ALIGN_UP (address);

  if (loom_sub (length, tmp - address, &length))
    return;

  length = CHUNK_ALIGN_DOWN (length);

  address = tmp;

  modend = loom_modend_get ();

  if (address < loom_modbase && address + length > loom_modbase)
    {
      exit = 1;
      loom_mm_add_region (address, loom_modbase - address);
    }

  if (address < modend && address + length > modend)
    {
      exit = 1;
      loom_mm_add_region (modend, (address + length) - modend);
    }

  if (exit || length < CHUNK_SIZE * 2 + MIN_ALLOC)
    return;

  free_chunk_t *fchunk = (free_chunk_t *) (address + MIN_ALLOC);

  *fchunk = (free_chunk_t) {
    .base = { .prev_size = CHUNK_FLAG_INUSE, .size = length - MIN_ALLOC },
    .prev = NULL,
    .next = NULL,
  };

  arena_t *arena = (arena_t *) address;
  arena->length = length;
  arena->freelist = fchunk;
  arena->next = arenas;
  arena->magic = ARENA_MAGIC_OF (arena);

  arenas = arena;
}

static void
chunk_update_next (arena_t *arena, chunk_t *chunk, loom_bool_t differ)
{
  chunk_t *nchunk;
  loom_usize_t chunk_size = chunk->size & CHUNK_FLAG_MASK;

  loom_address_t address = (loom_address_t) chunk,
                 arena_end = (loom_address_t) arena + arena->length;

  // If we either overflow the address space or the arena,
  // something went wrong.
  if (loom_add (address, chunk_size, &address) || address > arena_end)
    heap_panic ();

  // We are the last chunk. There is no next chunk to update.
  if (address == arena_end)
    return;

  // Minimum MIN_ALLOC per free_chunk_t or allocation.
  // If we have less than that, something went wrong.
  if (arena_end - address < MIN_ALLOC)
    heap_panic ();

  // If our address is misaligned, something went wrong.
  if (address & CHUNK_SIZE_MINUS_1)
    heap_panic ();

  nchunk = (chunk_t *) address;

  int i1 = chunk->size & CHUNK_FLAG_INUSE,
      i2 = nchunk->prev_size & CHUNK_FLAG_INUSE;

  if ((differ && i1 == i2) || (!differ && i1 != i2))
    heap_panic ();

  nchunk->prev_size = chunk->size;
}

static void
free_chunk_link (arena_t *arena, free_chunk_t *fchunk)
{
  fchunk->prev = NULL;
  fchunk->next = arena->freelist;

  if (arena->freelist)
    {
      if (arena->freelist->prev)
        heap_panic ();

      arena->freelist->prev = fchunk;
    }

  arena->freelist = fchunk;
  arena->magic = ARENA_MAGIC_OF (arena);
}

static void
free_chunk_unlink (arena_t *arena, free_chunk_t *fchunk)
{
  if (fchunk->prev)
    fchunk->prev->next = fchunk->next;
  else
    {
      if (arena->freelist != fchunk)
        heap_panic ();

      arena->freelist = fchunk->next;
      arena->magic = ARENA_MAGIC_OF (arena);
    }

  if (fchunk->next)
    fchunk->next->prev = fchunk->prev;

  fchunk->prev = fchunk->next = NULL;
}

static void *
free_chunk_split (arena_t *arena, free_chunk_t *fchunk, loom_usize_t offset,
                  loom_usize_t size)
{
  chunk_t *new_chunk;
  loom_usize_t chunk_size, flags;

  if (offset & CHUNK_SIZE_MINUS_1 || (offset && offset < MIN_ALLOC))
    loom_panic ("free_chunk_split: invalid offset %lu",
                (unsigned long) offset);

  if (!size || size & CHUNK_SIZE_MINUS_1)
    loom_panic ("free_chunk_split: invalid size %lu", (unsigned long) size);

  flags = fchunk->base.size & ~CHUNK_FLAG_MASK;

  if (flags & CHUNK_FLAG_INUSE)
    loom_panic ("free_chunk_split: invalid chunk with in use flag");

  chunk_size = fchunk->base.size & CHUNK_FLAG_MASK;

  if (offset >= chunk_size)
    loom_panic ("free_chunk_split: offset %lu >= chunk size %lu",
                (unsigned long) offset, (unsigned long) chunk_size);

  if (offset)
    {
      new_chunk = (chunk_t *) loom_assume_aligned ((char *) fchunk + offset,
                                                   loom_alignof (chunk_t));

      chunk_size -= offset;

      *new_chunk = (chunk_t) {
        .prev_size = (fchunk->base.size = offset | flags),
      };
    }
  else
    {
      free_chunk_unlink (arena, fchunk);
      new_chunk = &fchunk->base;
    }

  if (chunk_size < size)
    loom_panic ("free_chunk_split: chunk size too small");

  if (chunk_size - size >= MIN_ALLOC)
    {
      free_chunk_t *nfchunk = (free_chunk_t *) loom_assume_aligned (
          (char *) new_chunk + size, loom_alignof (free_chunk_t));

      new_chunk->size = size | flags | CHUNK_FLAG_INUSE;

      *nfchunk
          = (free_chunk_t) { .base = { .prev_size = new_chunk->size,
                                       .size = (chunk_size - size) | flags } };

      free_chunk_link (arena, nfchunk);

      chunk_update_next (arena, &nfchunk->base, 0);
    }
  else
    {
      new_chunk->size = chunk_size | flags | CHUNK_FLAG_INUSE;
      chunk_update_next (arena, new_chunk, 1);
    }

  return (char *) new_chunk + CHUNK_SIZE;
}

void *
loom_malloc (loom_usize_t size)
{
  if (loom_add (size, CHUNK_SIZE_MINUS_1, &size)
      || ((size &= ~CHUNK_SIZE_MINUS_1), loom_add (size, CHUNK_SIZE, &size)))
    {
      loom_error (LOOM_ERR_OVERFLOW, "requested allocation size is too big");
      return NULL;
    }

  LOOM_LIST_ITERATE (arenas, arena)
  {
    free_chunk_t *fchunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heap_panic ();

    fchunk = arena->freelist;

    while (fchunk)
      {
        loom_usize_t chunk_size;

        if (fchunk->base.size & CHUNK_FLAG_INUSE)
          heap_panic ();

        chunk_size = fchunk->base.size & CHUNK_FLAG_MASK;

        if (chunk_size >= size)
          return free_chunk_split (arena, fchunk, 0, size);

        fchunk = fchunk->next;
      }
  }

  return NULL;
}

void *
loom_zalloc (loom_usize_t size)
{
  void *p = loom_malloc (size);

  if (!p)
    return p;

  loom_memset (p, 0, size);

  return p;
}

void *
loom_calloc (loom_usize_t n, loom_usize_t size)
{
  if (!size)
    return NULL;

  if (loom_mul (n, size, &size))
    return NULL;

  return loom_zalloc (size);
}

void *
loom_realloc (void *p, loom_usize_t n)
{
  loom_usize_t copy, oldsize;
  loom_address_t address = (loom_address_t) p;

  chunk_t *chunk;
  void *new = loom_malloc (n);

  if (!new)
    return NULL;
  else if (!p)
    return new;

  if (address & CHUNK_SIZE_MINUS_1)
    loom_panic ("realloc: invalid pointer %p", p);

  // This is undefined behavior if p
  // is misaligned or is an invalid pointer.
  chunk = (chunk_t *) loom_assume_aligned ((char *) p - CHUNK_SIZE,
                                           loom_alignof (chunk_t));

  // Note: Some garbage might get copied if the old allocation contained an end
  // chunk that couldn't fit a free chunk. Shouldn't matter.
  oldsize = (chunk->size & CHUNK_FLAG_MASK) - CHUNK_SIZE;
  copy = n > oldsize ? oldsize : n;

  loom_memcpy (new, p, copy);

  // Let free do all the error checking here.
  loom_free (p);

  return new;
}

void *
loom_memalign (loom_usize_t size, loom_usize_t align)
{
  return loom_memalign_range (size, align, 0, LOOM_ADDRESS_MAX);
}

void *
loom_memalign_range (loom_usize_t size, loom_usize_t align,
                     loom_address_t min_addr, loom_address_t max_addr)
{
  if (loom_add (size, CHUNK_SIZE_MINUS_1, &size)
      || ((size &= ~CHUNK_SIZE_MINUS_1), loom_add (size, CHUNK_SIZE, &size)))
    {
      loom_error (LOOM_ERR_OVERFLOW, "requested allocation size is too big");
      return NULL;
    }

  if (min_addr > max_addr || size > max_addr - min_addr)
    {
      loom_error (LOOM_ERR_BAD_ARG,
                  "memalign range cannot fit allocation of size %lu",
                  (unsigned long) size);
      return NULL;
    }

  if (align & (align - 1))
    {
      loom_error (LOOM_ERR_BAD_ARG, "invalid requested alignment %lu",
                  (unsigned long) align);
      return NULL;
    }

  if (align <= CHUNK_SIZE)
    return loom_malloc (size);

  LOOM_LIST_ITERATE (arenas, arena)
  {
    free_chunk_t *fchunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heap_panic ();

    fchunk = arena->freelist;

    while (fchunk)
      {
        loom_address_t start = (loom_address_t) fchunk, end;
        loom_usize_t chunk_size = fchunk->base.size & CHUNK_FLAG_MASK;

        if (loom_add (start, chunk_size, &end)
            || fchunk->base.size & CHUNK_FLAG_INUSE)
          heap_panic ();

        if (start < min_addr)
          start = min_addr;

        start = (start + (align - 1)) & ~(align - 1);

        while (start < LOOM_USIZE_MAX - size && start + size <= max_addr
               && start < end && end - start >= size)
          {
            loom_usize_t offset;

            if (start - (loom_address_t) fchunk < MIN_ALLOC
                || loom_sub (start, (loom_address_t) fchunk, &offset)
                || loom_sub (offset, CHUNK_SIZE, &offset))
              {
                if (loom_add (start, align, &start))
                  break;
                continue;
              }

            return free_chunk_split (arena, fchunk, offset, size);
          }

        fchunk = fchunk->next;
      }
  }

  return NULL;
}

void
loom_free (void *p)
{
  loom_address_t address = (loom_address_t) p;

  if (p == NULL)
    return;

  if (loom_sub (address, CHUNK_SIZE, &address) || address & CHUNK_SIZE_MINUS_1
      || address > LOOM_ADDRESS_MAX - MIN_ALLOC)
    loom_panic ("free: invalid pointer %p", p);

  LOOM_LIST_ITERATE (arenas, arena)
  {
    loom_address_t arena_start, arena_end, chunk_end;
    chunk_t *chunk;
    free_chunk_t *fchunk;
    loom_bool_t inuse, differ = 1;

    if (ARENA_MAGIC_OF (arena) != arena->magic)
      heap_panic ();

    arena_start = (loom_address_t) arena + MIN_ALLOC;
    arena_end = (loom_address_t) arena + arena->length;

    if (address < arena_start || address + MIN_ALLOC > arena_end)
      continue;

    chunk = (chunk_t *) address;

    // Chunk should be allocated. If not, we got
    // a bad pointer or there is heap corruption.
    if (!(chunk->size & CHUNK_FLAG_INUSE))
      heap_panic ();

    fchunk = (free_chunk_t *) chunk;
    fchunk->base.size &= ~CHUNK_FLAG_INUSE;

    // Speculatively link the free chunk.
    // If we perform any backward coalescing, we unlink this.
    free_chunk_link (arena, fchunk);

    inuse = fchunk->base.prev_size & CHUNK_FLAG_INUSE;

    // Perform backward coalescing.
    while (!inuse)
      {
        free_chunk_t *bchunk;
        loom_usize_t prev_size = fchunk->base.prev_size & CHUNK_FLAG_MASK,
                     size;

        if (prev_size < MIN_ALLOC || loom_sub (address, prev_size, &address)
            || address < arena_start)
          heap_panic ();

        bchunk = (free_chunk_t *) address;
        if (bchunk->base.size & CHUNK_FLAG_INUSE
            || (bchunk->base.size & CHUNK_FLAG_MASK) != prev_size)
          heap_panic ();

        size = fchunk->base.size & CHUNK_FLAG_MASK;

        free_chunk_unlink (arena, fchunk);

        // Grow back chunk.
        loom_usize_t flags = bchunk->base.size & ~CHUNK_FLAG_MASK;
        bchunk->base.size = (prev_size + size) | flags;

        fchunk = bchunk;
        inuse = bchunk->base.prev_size & CHUNK_FLAG_INUSE;
      }

    chunk_end
        = (loom_address_t) fchunk + (fchunk->base.size & CHUNK_FLAG_MASK);

    inuse = 0;

    // Coalesce forward.
    while (chunk_end < arena_end)
      {
        free_chunk_t *nchunk;
        loom_usize_t size = fchunk->base.size & CHUNK_FLAG_MASK;

        chunk = (chunk_t *) chunk_end;
        if (chunk->size & CHUNK_FLAG_INUSE)
          {
            inuse = 1;
            break;
          }

        differ = 0;

        nchunk = (free_chunk_t *) chunk;

        free_chunk_unlink (arena, nchunk);

        loom_usize_t flags = fchunk->base.size & ~CHUNK_FLAG_MASK,
                     newsize = nchunk->base.size & CHUNK_FLAG_MASK;

        if (newsize < MIN_ALLOC || loom_add (size, newsize, &size)
            || loom_add (chunk_end, newsize, &chunk_end))
          heap_panic ();

        fchunk->base.size = size | flags;
      }

    // Either heap corruption or some logic error.
    if (chunk_end != arena_end && !inuse)
      heap_panic ();

    chunk_update_next (arena, &fchunk->base, differ);

    // All done.
    return;
  }

  loom_panic ("free: invalid pointer %p", p);
}

int
loom_mm_iterate (int (*hook) (loom_address_t p, loom_usize_t n,
                              loom_bool_t isfree, void *data),
                 void *data)
{
  if (!hook)
    {
      loom_error (LOOM_ERR_BAD_ARG, "invalid memory manager iterate hook %p",
                  hook);
      return -1;
    }

  // Note: This serves as a way of validating the heap as well as
  // hooking and iterating free chunks and allocations.
  LOOM_LIST_ITERATE (arenas, arena)
  {
    loom_address_t address = (loom_address_t) arena, arena_end;
    loom_usize_t prev_size;
    loom_bool_t first = 1;
    chunk_t *chunk;

    if (ARENA_MAGIC_OF (arena) != arena->magic
        || loom_add (address, arena->length, &arena_end))
      heap_panic ();

    address += MIN_ALLOC;

    while (address < arena_end)
      {
        int retval;

        loom_usize_t chunk_size;
        chunk = (chunk_t *) address;

        chunk_size = chunk->size & CHUNK_FLAG_MASK;

        if (chunk_size < MIN_ALLOC || (chunk_size & CHUNK_SIZE_MINUS_1)
            || loom_add (address, chunk_size, &address) || address > arena_end
            || (!first && prev_size != chunk->prev_size))
          heap_panic ();

        retval
            = hook (address - chunk_size + CHUNK_SIZE, chunk_size - CHUNK_SIZE,
                    !(chunk->size & CHUNK_FLAG_INUSE), data);

        if (retval)
          return retval;

        first = 0;
        prev_size = chunk->size;
      }

    if (address != arena_end)
      heap_panic ();
  }

  return 0;
}