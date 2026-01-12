#ifndef LOOM_I686_ISR_H
#define LOOM_I686_ISR_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef void (*isr_handler) (loom_uint32_t intno, loom_uint32_t error_code);

void EXPORT (loom_isr_wrapper) (void);
void NORETURN EXPORT (loom_exception_handler) (loom_uint32_t intno,
                                               loom_uint32_t error_code);

#endif