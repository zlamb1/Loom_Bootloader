#ifndef LOOM_CONSOLE_H
#define LOOM_CONSOLE_H 1

#include "loom/assert.h"
#include "loom/compiler.h"
#include "loom/error.h"
#include "loom/list.h"

#define LOOM_CONSOLE_COLOR_BLACK     0x0
#define LOOM_CONSOLE_COLOR_RED       0x1
#define LOOM_CONSOLE_COLOR_GREEN     0x2
#define LOOM_CONSOLE_COLOR_YELLOW    0x3
#define LOOM_CONSOLE_COLOR_BLUE      0x4
#define LOOM_CONSOLE_COLOR_MAGENTA   0x5
#define LOOM_CONSOLE_COLOR_CYAN      0x6
#define LOOM_CONSOLE_COLOR_WHITE     0x7
#define LOOM_CONSOLE_COLOR_MAX       0xF
#define LOOM_CONSOLE_COLOR_BRIGHT(X) (X + 8)

#define LOOM_CONSOLE_DEFAULT_FG                                               \
  LOOM_CONSOLE_COLOR_BRIGHT (LOOM_CONSOLE_COLOR_WHITE)
#define LOOM_CONSOLE_DEFAULT_BG LOOM_CONSOLE_COLOR_BLACK

typedef u8 loom_console_color;

typedef struct
{
  usize len, splats;
  const char *s;
} loom_write_buffer;

typedef struct loom_console
{
  usize (*get_x) (struct loom_console *);
  usize (*get_y) (struct loom_console *);
  u8 (*get_fg) (struct loom_console *);
  u8 (*get_bg) (struct loom_console *);
  loom_error (*set_x) (struct loom_console *, usize);
  loom_error (*set_y) (struct loom_console *, usize);
  loom_error (*set_fg) (struct loom_console *, u8);
  loom_error (*set_bg) (struct loom_console *, u8);
  void (*clear) (struct loom_console *);
  void (*write_all) (struct loom_console *, loom_write_buffer[]);

  void *data;
  loom_console_color save;

  loom_list node;
} loom_console;

extern loom_list loom_consoles;

void loomWbufsPrepend (usize cap, loom_write_buffer wbufs[],
                       loom_write_buffer wbuf);
void loomWbufsAppend (usize cap, loom_write_buffer wbufs[],
                      loom_write_buffer wbuf);
usize loomWbufsCharLen (loom_write_buffer wbufs[]);

void export (loomConsoleRegister) (loom_console *console);
void export (loomConsoleUnregister) (loom_console *console);

void export (loomConsolesClear) (void);
void loomConsolesWrite (usize len, const char *buf);
void loomConsolesWriteStr (const char *s);
void loomConsolesWriteAll (loom_write_buffer wbufs[]);

static inline void
loomConsolesSetFg (loom_console_color fg)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_fg != NULL);
    console->set_fg (console, fg);
  }
}

static inline void
loomConsolesSaveFg (void)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->get_fg != NULL);
    console->save = console->get_fg (console);
  }
}

static inline void
loomConsolesRestoreFg (void)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_fg != NULL);
    console->set_fg (console, console->save);
  }
}

static inline void
loomConsolesSetBg (loom_console_color bg)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_bg != NULL);
    console->set_bg (console, bg);
  }
}

static inline void
loomConsolesSaveBg (void)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->get_bg != NULL);
    console->save = console->get_bg (console);
  }
}

static inline void
loomConsolesRestoreBg (void)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_bg != NULL);
    console->set_bg (console, console->save);
  }
}

#endif