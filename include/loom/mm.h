#ifndef LOOM_MM_H
#define LOOM_MM_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loomMMAddRegion (usize address, usize length);

void *export (loomAlloc) (usize size);
void *export (loomZeroAlloc) (usize size);
void *export (loomArrayAlloc) (usize n, usize size);
void *export (loomRealloc) (void *p, usize newsize);

void *export (loomMemAlign) (usize size, usize align);
void *export (loomMemAlignRange) (usize size, usize align, address min_addr,
                                  address max_addr);

void export (loomFree) (void *p);

int export (loomMMIterate) (int (*hook) (address p, usize n, bool is_free,
                                         void *data),
                            void *data);

#endif