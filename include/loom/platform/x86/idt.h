#ifndef LOOM_I686_IDT_H
#define LOOM_I686_IDT_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loomIdtInit (void);
void loomIdtrLoad (void);

void export (loomIsrVectorMap) (u8 entry, void *isr);
void export (loomIdtVectorMap) (u8 entry, void *isr);
void export (loomIdtVectorUnmap) (u8 entry);

#endif