#ifndef LOOM_I686_PIC_H
#define LOOM_I686_PIC_H 1

#include "loom/compiler.h"
#include "loom/platform/x86/bios/bios.h"
#include "loom/types.h"

void export (loomPICRemap) (u8 offset1, u8 offset2);
void export (loomPICMask) (u8 irq);
void export (loomPICUnmask) (u8 irq);
void export (loomPICAckIrq) (u8 irq);
void export (loomPICDisable) (void);
void export (loomPICRegisterIsr) (u8 irq, void *isr);

void loomPICSaveBiosDefaults (void);
void loomPICResetBiosDefaults (void);
void loomPICRestoreMasks (void);

extern loom_bios_hook loomPICBiosHook;

#endif