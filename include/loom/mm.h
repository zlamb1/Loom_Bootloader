#ifndef LOOM_MM_H
#define LOOM_MM_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loom_mm_add_region (usize address, usize length);

void *export (loom_malloc) (usize size);
void *export (loom_zalloc) (usize size);
void *export (loom_calloc) (usize n, usize size);
void *export (loom_realloc) (void *p, usize newsize);

void *export (loom_memalign) (usize size, usize align);
void *export (loom_memalign_range) (usize size, usize align, address min_addr,
                                    address max_addr);

void export (loom_free) (void *p);

int export (loom_mm_iterate) (int (*hook) (address p, usize n, bool is_free,
                                           void *data),
                              void *data);

#endif