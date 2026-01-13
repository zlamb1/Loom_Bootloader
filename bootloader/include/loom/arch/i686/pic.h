#ifndef LOOM_I686_PIC_H
#define LOOM_I686_PIC_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void EXPORT (loom_pic_remap) (loom_uint8_t offset1, loom_uint8_t offset2);
void EXPORT (loom_pic_mask) (loom_uint8_t irq);
void EXPORT (loom_pic_unmask) (loom_uint8_t irq);
void EXPORT (loom_pic_eoi) (loom_uint8_t irq);
void EXPORT (loom_pic_disable) (void);
void EXPORT (loom_pic_register_isr) (loom_uint8_t irq, void *isr);

#endif