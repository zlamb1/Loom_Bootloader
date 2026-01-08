#ifndef LOOM_CONSOLE_H
#define LOOM_CONSOLE_H 1

#include "compiler.h"
#include "error.h"

#define LOOM_CONSOLE_COLOR_BLACK 0
#define LOOM_CONSOLE_COLOR_BLUE 1
#define LOOM_CONSOLE_COLOR_GREEN 2
#define LOOM_CONSOLE_COLOR_CYAN 3
#define LOOM_CONSOLE_COLOR_RED 4
#define LOOM_CONSOLE_COLOR_PURPLE 5
#define LOOM_CONSOLE_COLOR_BROWN 6
#define LOOM_CONSOLE_COLOR_LIGHT_GRAY 7
#define LOOM_CONSOLE_COLOR_DARK_GRAY 8
#define LOOM_CONSOLE_COLOR_LIGHT_BLUE 9
#define LOOM_CONSOLE_COLOR_LIGHT_GREEN 10
#define LOOM_CONSOLE_COLOR_LIGHT_CYAN 11
#define LOOM_CONSOLE_COLOR_LIGHT_RED 12
#define LOOM_CONSOLE_COLOR_LIGHT_PURPLE 13
#define LOOM_CONSOLE_COLOR_YELLOW 14
#define LOOM_CONSOLE_COLOR_WHITE 15
#define LOOM_CONSOLE_COLOR_MAX 15

typedef struct loom_console {
    loom_usize (*get_x)(struct loom_console *);
    loom_usize (*get_y)(struct loom_console *);
    loom_u8 (*get_fg)(struct loom_console *);
    loom_u8 (*get_bg)(struct loom_console *);
    loom_error (*set_x)(struct loom_console *, loom_usize);
    loom_error (*set_y)(struct loom_console *, loom_usize);
    loom_error (*set_fg)(struct loom_console *, loom_u8);
    loom_error (*set_bg)(struct loom_console *, loom_u8);
    void (*clear)(struct loom_console *);
    void (*write)(struct loom_console *, loom_usize, const char *);

    void *data;
    struct loom_console *next;
} loom_console;

extern loom_console *consoles;

void loom_con_register(loom_console *con);

void loom_con_write(loom_usize len, const char *buf);

#endif