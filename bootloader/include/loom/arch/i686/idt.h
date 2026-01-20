#ifndef LOOM_I686_IDT_H
#define LOOM_I686_IDT_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loom_idt_init (void);
void loom_idtr_load (void);

void EXPORT (loom_isr_vector_map) (loom_uint8_t entry, void *isr);
void EXPORT (loom_idt_vector_map) (loom_uint8_t entry, void *isr);
void EXPORT (loom_idt_vector_unmap) (loom_uint8_t entry);

#endif