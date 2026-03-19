#ifndef LOOM_I686_IDT_H
#define LOOM_I686_IDT_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loom_idt_init (void);
void loom_idtr_load (void);

void export (loom_isr_vector_map) (u8 entry, void *isr);
void export (loom_idt_vector_map) (u8 entry, void *isr);
void export (loom_idt_vector_unmap) (u8 entry);

#endif