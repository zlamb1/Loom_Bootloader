#include "loom/console.h"

loom_console *consoles = NULL;

void loom_con_register(loom_console *con) {
    con->set_fg(con, LOOM_CONSOLE_COLOR_WHITE);
    con->set_bg(con, LOOM_CONSOLE_COLOR_BLACK);
    con->clear(con);

    con->next = consoles;
    consoles = con;
}

void loom_con_write(loom_usize len, const char *buf) {
    loom_console *con = consoles;

    while (con != NULL) {
        con->write(con, len, buf);
        con = con->next;
    }
}