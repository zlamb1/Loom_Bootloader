#ifndef LOOM_MM_H
#define LOOM_MM_H 1

#include "compiler.h"
#include "types.h"

void loom_mm_add_region (loom_usize_t address, loom_usize_t length);

void *EXPORT (loom_malloc) (loom_usize_t size);
void *EXPORT (loom_zalloc) (loom_usize_t size);
void *EXPORT (loom_calloc) (loom_usize_t n, loom_usize_t size);
void *EXPORT (loom_realloc) (void *p, loom_usize_t newsize);

void EXPORT (loom_free) (void *p);

int EXPORT (loom_mm_iterate) (int (*hook) (loom_address_t p, loom_usize_t n,
                                           loom_bool_t isfree, void *data),
                              void *data);

#endif