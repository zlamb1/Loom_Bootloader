#ifndef LOOM_MM_H
#define LOOM_MM_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loomHeapAddRegion (usize address, usize length);

void *export (loomAlloc) (usize size);
void *export (loomZeroAlloc) (usize size);
void *export (loomArrayAlloc) (usize n, usize size);
void *export (loomRealloc) (void *p, usize newsize);

void *export (loomMemAlign) (usize size, usize align);
void *export (loomMemAlignRange) (usize size, usize align, address min_addr,
                                  address max_addr);

/*
 * NOTE: this function scans for a best-fit at the highest possible address.
 * As such, it should be used sparingly as it has to scan the whole heap.
 */
void *export (loomMemAlignRangeHigh) (usize size, usize align,
                                      address min_addr, address max_addr);

void export (loomFree) (void *p);

int export (loomHeapIterate) (int (*hook) (address p, usize n, bool is_free,
                                           void *data),
                              void *data);

#endif