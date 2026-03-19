#ifndef LOOM_I686_IO_H
#define LOOM_I686_IO_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void export (loom_outb) (u16 port, u8 val);
u8 export (loom_inb) (u16 port);

#endif