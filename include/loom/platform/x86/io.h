#ifndef LOOM_I686_IO_H
#define LOOM_I686_IO_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void export (loomOutByte) (u16 port, u8 val);
u8 export (loomInByte) (u16 port);

#endif