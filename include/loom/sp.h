#ifndef LOOM_SP_H
#define LOOM_SP_H 1

#include "loom/compiler.h"
#include "loom/types.h"

extern uintptr export_var (__stack_chk_guard);

void noreturn export (__stack_chk_fail) (void);

#endif