#ifndef LOOM_I686_PIC_H
#define LOOM_I686_PIC_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void export (loom_pic_remap) (u8 offset1, u8 offset2);
void export (loom_pic_mask) (u8 irq);
void export (loom_pic_unmask) (u8 irq);
void export (loom_pic_eoi) (u8 irq);
void export (loom_pic_disable) (void);
void export (loom_pic_register_isr) (u8 irq, void *isr);

void loom_pic_bios_save_masks (void);

void loom_pic_bios_reset (void);
void loom_pic_bios_restore (void);

#endif