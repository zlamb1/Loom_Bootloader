#ifndef LOOM_I686_IO_H
#define LOOM_I686_IO_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void LOOM_EXPORT (loom_outb) (loom_uint16_t port, loom_uint8_t val);
loom_uint8_t LOOM_EXPORT (loom_inb) (loom_uint16_t port);

#endif