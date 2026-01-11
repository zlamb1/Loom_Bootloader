#ifndef LOOM_CONSOLE_H
#define LOOM_CONSOLE_H 1

#include "error.h"

#define LOOM_CONSOLE_COLOR_BLACK        0
#define LOOM_CONSOLE_COLOR_BLUE         1
#define LOOM_CONSOLE_COLOR_GREEN        2
#define LOOM_CONSOLE_COLOR_CYAN         3
#define LOOM_CONSOLE_COLOR_RED          4
#define LOOM_CONSOLE_COLOR_PURPLE       5
#define LOOM_CONSOLE_COLOR_BROWN        6
#define LOOM_CONSOLE_COLOR_LIGHT_GRAY   7
#define LOOM_CONSOLE_COLOR_DARK_GRAY    8
#define LOOM_CONSOLE_COLOR_LIGHT_BLUE   9
#define LOOM_CONSOLE_COLOR_LIGHT_GREEN  10
#define LOOM_CONSOLE_COLOR_LIGHT_CYAN   11
#define LOOM_CONSOLE_COLOR_LIGHT_RED    12
#define LOOM_CONSOLE_COLOR_LIGHT_PURPLE 13
#define LOOM_CONSOLE_COLOR_YELLOW       14
#define LOOM_CONSOLE_COLOR_WHITE        15
#define LOOM_CONSOLE_COLOR_MAX          15

typedef struct
{
  loom_usize_t len, splats;
  const char *s;
} loom_write_buffer_t;

typedef struct loom_console_t
{
  loom_usize_t (*get_x) (struct loom_console_t *);
  loom_usize_t (*get_y) (struct loom_console_t *);
  loom_uint8_t (*get_fg) (struct loom_console_t *);
  loom_uint8_t (*get_bg) (struct loom_console_t *);
  loom_error_t (*set_x) (struct loom_console_t *, loom_usize_t);
  loom_error_t (*set_y) (struct loom_console_t *, loom_usize_t);
  loom_error_t (*set_fg) (struct loom_console_t *, loom_uint8_t);
  loom_error_t (*set_bg) (struct loom_console_t *, loom_uint8_t);
  void (*clear) (struct loom_console_t *);
  void (*write_all) (struct loom_console_t *, loom_write_buffer_t[]);

  void *data;
  struct loom_console_t *next;
} loom_console_t;

extern loom_console_t *consoles;

void loom_wbufs_prepend (loom_usize_t cap, loom_write_buffer_t wbufs[],
                         loom_write_buffer_t wbuf);
void loom_wbufs_append (loom_usize_t cap, loom_write_buffer_t wbufs[],
                        loom_write_buffer_t wbuf);
loom_usize_t loom_wbufs_char_len (loom_write_buffer_t wbufs[]);

void loom_con_register (loom_console_t *con);
void loom_con_clear (void);
void loom_con_write (loom_usize_t len, const char *buf);
void loom_con_write_str (const char *s);
void loom_con_write_all (loom_write_buffer_t wbufs[]);

#endif