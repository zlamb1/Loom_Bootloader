#ifndef LOOM_I686_ISR_H
#define LOOM_I686_ISR_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef void (*isr_handler) (u32 intno, u32 error_code);

void LOOM_EXPORT (loom_isr_wrapper) (void);
void LOOM_NORETURN LOOM_EXPORT (loom_exception_handler) (u32 intno,
                                                         u32 error_code);

#endif