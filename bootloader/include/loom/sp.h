#ifndef LOOM_SP_H
#define LOOM_SP_H 1

#include "loom/compiler.h"
#include "loom/types.h"

extern loom_uintptr_t LOOM_EXPORT_VAR (__stack_chk_guard);

void LOOM_NORETURN LOOM_EXPORT (__stack_chk_fail) (void);

#endif