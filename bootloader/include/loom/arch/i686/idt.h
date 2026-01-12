#ifndef LOOM_I686_IDT_H
#define LOOM_I686_IDT_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void loom_idt_init (void);
void loom_load_idtr (void);

void EXPORT (loom_map_isr_vector) (loom_uint8_t entry, void *isr);
void EXPORT (loom_map_idt_vector) (loom_uint8_t entry, void *isr);

#endif