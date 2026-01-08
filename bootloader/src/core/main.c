#include "loom/compiler.h"
#include "loom/arch.h"
#include "loom/console.h"

void NORETURN loom_main(void) {
    loom_arch_init();
    loom_con_write(6, "Hello!");

    for (;;);
}