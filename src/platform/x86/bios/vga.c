#include "loom/console.h"
#include "loom/platform/x86/io.h"

#define ROWS 25
#define COLS 80

#define VMEM_MAX (ROWS * COLS)

#define CRTC_ADDRESS_REG         0x3D4
#define CRTC_DATA_REG            0x3D5
#define CRTC_CURSOR_HIGH_LOC_REG 0x00E
#define CRTC_CURSOR_LOW_LOC_REG  0x00F

volatile u16 *VMEM = (volatile u16 *) 0xB8000;

typedef struct
{
  usize x, y;
  u8 attribs;
  loom_console super;
} loom_vga_console;

static u8 console_color_vga_map[8] = {
  [LOOM_CONSOLE_COLOR_BLACK] = 0x0,  [LOOM_CONSOLE_COLOR_BLUE] = 0x1,
  [LOOM_CONSOLE_COLOR_GREEN] = 0x2,  [LOOM_CONSOLE_COLOR_CYAN] = 0x3,
  [LOOM_CONSOLE_COLOR_RED] = 0x4,    [LOOM_CONSOLE_COLOR_MAGENTA] = 0x5,
  [LOOM_CONSOLE_COLOR_YELLOW] = 0x6, [LOOM_CONSOLE_COLOR_WHITE] = 0x7,
};

static inline u16
loom_vga_convert_attribs (u16 attribs)
{
  u8 fg = (u8) attribs;
  u8 bg = (u8) (attribs >> 8);

  if (fg > 0xF)
    fg = 0;

  if (bg > 0xF)
    bg = 0;

  fg = fg > 7 ? console_color_vga_map[fg & 7] | 8 : console_color_vga_map[fg];
  bg = bg > 7 ? console_color_vga_map[bg & 7] | 8 : console_color_vga_map[bg];

  return (u16) (fg | (bg << 4));
}

static void
loom_vga_sync_cursor (usize x, usize y)
{
  u16 pos = (u16) (y * COLS + x);
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_LOW_LOC_REG);
  loom_outb (CRTC_DATA_REG, (u8) (pos & 0xFF));
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_HIGH_LOC_REG);
  loom_outb (CRTC_DATA_REG, (u8) ((pos >> 8) & 0xFF));
}

static usize
loom_vga_get_x (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->x;
}

static usize
loom_vga_get_y (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->y;
}

static u8
loom_vga_get_fg (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->attribs & 0xF;
}

static u8
loom_vga_get_bg (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return (vga_con->attribs >> 4) & 0xF;
}

static loom_error
loom_vga_set_x (loom_console *con, usize x)
{
  if (x >= COLS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->x = x;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_y (loom_console *con, usize y)
{
  if (y >= ROWS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->y = y;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_fg (loom_console *con, u8 fg)
{
  if (fg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (vga_con->attribs & 0xF0) | fg;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_bg (loom_console *con, u8 bg)
{
  if (bg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (u8) ((vga_con->attribs & 0xF) | (bg << 4));

  return LOOM_ERR_NONE;
}

static void
loom_vga_clear (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  u16 char_and_attribs
      = (u16) ' ' | (u16) (loom_vga_convert_attribs (vga_con->attribs) << 8);

  for (usize i = 0; i < ROWS * COLS; ++i)
    VMEM[i] = char_and_attribs;

  vga_con->x = 0;
  vga_con->y = 0;
  loom_vga_sync_cursor (0, 0);
}

static void
loom_vga_scroll (loom_console *con)
{
  (void) con;
}

static usize
loom_vga_write_wbuf (loom_console *con, loom_write_buffer wbuf, u16 attribs,
                     usize index)
{
  for (usize windex = 0; windex < wbuf.len; ++windex)
    {
      char ch = wbuf.s[windex];

      switch (ch)
        {
        case '\b':
          if (index)
            --index;
          continue;
        case '\r':
          index = (index / COLS) * COLS;
          continue;
        case '\n':
          index = ((index + COLS) / COLS) * COLS;
          goto maybe_scroll;
        default:
          break;
        }

      VMEM[index++] = (u16) ch | attribs;

    maybe_scroll:
      if (index >= VMEM_MAX)
        {
          if (index > VMEM_MAX)
            loom_panic ("loom_vga_write_wbuf");

          index = (ROWS - 1) * COLS;
          loom_vga_scroll (con);
        }
    }

  return index;
}

static void
loom_vga_write_all (struct loom_console *con, loom_write_buffer wbufs[])
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  usize index = vga_con->y * COLS + vga_con->x;
  u16 attribs = (u16) (loom_vga_convert_attribs (vga_con->attribs) << 8);
  loom_write_buffer wbuf;

  if (vga_con->x >= COLS || vga_con->y >= ROWS)
    loom_panic ("loom_vga_write_all");

  for (int i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];

      for (usize j = 0; j < wbuf.splats; ++j)
        index = loom_vga_write_wbuf (con, wbuf, attribs, index);
    }

  vga_con->x = index % COLS;
  vga_con->y = index / COLS;

  loom_vga_sync_cursor (vga_con->x, vga_con->y);
}

static loom_vga_console vga_con = {
    .x = 0,
    .y = 0,
    .attribs = 0,
    .super = {
        .get_x = loom_vga_get_x,
        .get_y = loom_vga_get_y,
        .get_fg = loom_vga_get_fg,
        .get_bg = loom_vga_get_bg,
        .set_x = loom_vga_set_x,
        .set_y = loom_vga_set_y,
        .set_fg = loom_vga_set_fg,
        .set_bg = loom_vga_set_bg,
        .clear = loom_vga_clear,
        .write_all = loom_vga_write_all,
        .data = &vga_con,
    },
};

void
loom_vga_con_register (void)
{
  loom_console_register (&vga_con.super);
}