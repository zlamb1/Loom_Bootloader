#include <stddef.h>

#include "loom/error.h"
#include "loom/list.h"
#include "loom/mm.h"

#define ALIGN         _Alignof (max_align_t)
#define ALIGN_MINUS_1 (ALIGN - 1)
#define MAGIC         0xA110CA7E
#define MAGIC_OF(X)                                                           \
  ({                                                                          \
    arena_t *a = (X);                                                         \
    MAGIC ^ a->length ^ a->offset ^ (loom_uintptr_t) a->next;                 \
  })

#define ALIGN_DOWN(X) ((X) & ~ALIGN_MINUS_1)
#define ALIGN_UP(X)   ALIGN_DOWN ((X) + ALIGN_MINUS_1)

typedef struct arena_t
{
  loom_usize_t length;
  loom_usize_t offset;
  loom_usize_t prev_alloc;
  loom_uint32_t magic;
  struct arena_t *next;
} arena_t;

compile_assert (ALIGN / _Alignof (arena_t) >= 1,
                "arena_t alignment must match ALIGN.");

static arena_t *arenas = NULL;

void
loom_mm_add_region (loom_usize_t address, loom_usize_t length)
{
  loom_usize_t tmp;
  loom_usize_t aligned_arena_size;

  if (address > LOOM_USIZE_MAX - length)
    length = LOOM_USIZE_MAX - address;

  if (address > LOOM_USIZE_MAX - ALIGN_MINUS_1)
    return;

  tmp = ALIGN_UP (address);

  if (tmp - address >= length)
    return;

  length -= tmp - address;
  length = ALIGN_DOWN (length);

  address = tmp;

  aligned_arena_size = ALIGN_UP (sizeof (arena_t));

  if (length < aligned_arena_size + ALIGN)
    return;

  arena_t *arena = (arena_t *) address;
  arena->length = length - aligned_arena_size;
  arena->offset = aligned_arena_size;
  arena->prev_alloc = 0;
  arena->next = arenas;
  arena->magic = MAGIC_OF (arena);

  arenas = arena;
}

loom_usize_t
loom_mm_bytes_free (void)
{
  loom_usize_t bytes = 0;

  LOOM_LIST_ITERATE (arenas, arena)
  {
    if (arena->magic != MAGIC_OF (arena))
      loom_panic ("heap corruption detected");
    bytes += arena->length;
  }

  return bytes;
}

loom_usize_t
loom_mm_bytes_allocated (void)
{
  loom_usize_t bytes = 0;

  LOOM_LIST_ITERATE (arenas, arena)
  {
    if (arena->magic != MAGIC_OF (arena))
      loom_panic ("heap corruption detected");
    bytes += arena->offset;
  }

  return bytes;
}

void *
loom_malloc (loom_usize_t size)
{
  arena_t *arena;

  if (!size)
    return NULL;

  if (size > LOOM_USIZE_MAX - ALIGN)
    return NULL;

  size = ALIGN_UP (size);
  arena = arenas;

  while (arena)
    {
      void *p;

      if (arena->magic != MAGIC_OF (arena))
        loom_panic ("heap corruption detected");

      if (arena->length < size)
        goto next;

      p = (char *) arena + arena->offset;

      arena->length -= size;
      arena->offset += size;
      arena->prev_alloc = size;
      arena->magic = MAGIC_OF (arena);

      return p;

    next:
      arena = arena->next;
    }

  return NULL;
}

void *
loom_zalloc (loom_usize_t size)
{
  void *p = loom_malloc (size);

  if (!p)
    return p;

  char *c = p;
  for (loom_usize_t i = 0; i < size; ++i)
    c[i] = 0;

  return p;
}

void *
loom_calloc (loom_usize_t n, loom_usize_t size)
{
  if (!size)
    return NULL;

  if (n > LOOM_USIZE_MAX / size)
    return NULL;

  size *= n;

  if (size > LOOM_USIZE_MAX - ALIGN_MINUS_1)
    return NULL;

  return loom_zalloc (size);
}

void *
loom_realloc (void *p, loom_usize_t oldsize, loom_usize_t newsize)
{
  loom_usize_t copy;
  void *np;
  char *c, *nc;

  if (!newsize)
    return NULL;

  if (!p)
    return loom_malloc (newsize);

  c = p;

  np = loom_malloc (newsize);
  nc = np;

  if (!np)
    return NULL;

  copy = newsize > oldsize ? oldsize : newsize;
  for (loom_usize_t i = 0; i < copy; ++i)
    nc[i] = c[i];

  loom_free (p);

  return np;
}

void
loom_free (void *p)
{
  loom_usize_t arena_aligned_size = ALIGN_UP (sizeof (arena_t));

  loom_uintptr_t addr = (loom_uintptr_t) p;

  if (addr != ALIGN_DOWN (addr))
    loom_panic ("invalid pointer");

  // Do nothing on NULL.
  if (!addr)
    return;

  LOOM_LIST_ITERATE (arenas, arena)
  {
    loom_uintptr_t arena_addr;

    if (arena->magic != MAGIC_OF (arena))
      loom_panic ("heap corruption detected");

    arena_addr = (loom_uintptr_t) arena;

    if (addr >= arena_addr + arena_aligned_size
        && addr < arena_addr + arena->offset)
      {
        if (addr + arena->prev_alloc == arena_addr + arena->offset)
          {
            arena->offset -= arena->prev_alloc;
            arena->length += arena->prev_alloc;
            arena->prev_alloc = 0;
            arena->magic = MAGIC_OF (arena);
          }

        return;
      }
  }

  loom_panic ("invalid pointer");
}