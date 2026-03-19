#ifndef LOOM_MM_H
#define LOOM_MM_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loom_mm_add_region (usize address, usize length);

void *LOOM_EXPORT (loom_malloc) (usize size);
void *LOOM_EXPORT (loom_zalloc) (usize size);
void *LOOM_EXPORT (loom_calloc) (usize n, usize size);
void *LOOM_EXPORT (loom_realloc) (void *p, usize newsize);

void *LOOM_EXPORT (loom_memalign) (usize size, usize align);
void *LOOM_EXPORT (loom_memalign_range) (usize size, usize align,
                                         address min_addr, address max_addr);

void LOOM_EXPORT (loom_free) (void *p);

int LOOM_EXPORT (loom_mm_iterate) (int (*hook) (address p, usize n,
                                                bool is_free, void *data),
                                   void *data);

#endif