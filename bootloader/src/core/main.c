#include "loom/compiler.h"

void NORETURN loom_main(void) {
    unsigned char *vmem = (unsigned char *) 0xB8000;
    vmem[0] = 'T';
    for (;;);
}