#include "loom/console.h"

#define ROWS 25
#define COLS 80

#define VMEM_MAX (ROWS * COLS)

#define CRTC_ADDRESS_REG         0x3D4
#define CRTC_DATA_REG            0x3D5
#define CRTC_CURSOR_HIGH_LOC_REG 0x00E
#define CRTC_CURSOR_LOW_LOC_REG  0x00F

volatile loom_u16 *VMEM = (volatile loom_u16 *) 0xB8000;

typedef struct
{
  loom_usize x, y;
  loom_u8 attribs;
  loom_console interface;
} loom_vga_console;

static void
loom_outb (loom_u16 port, loom_u8 v)
{
  __asm__ volatile ("outb %1, %0" ::"Nd"(port), "a"(v));
}

static void
loom_vga_sync_cursor (loom_usize x, loom_usize y)
{
  loom_u16 pos = (loom_u16) (y * COLS + x);
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_LOW_LOC_REG);
  loom_outb (CRTC_DATA_REG, (loom_u8) (pos & 0xFF));
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_HIGH_LOC_REG);
  loom_outb (CRTC_DATA_REG, (loom_u8) ((pos >> 8) & 0xFF));
}

static loom_usize
loom_vga_get_x (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->x;
}

static loom_usize
loom_vga_get_y (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->y;
}

static loom_u8
loom_vga_get_fg (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->attribs & 0xF;
}

static loom_u8
loom_vga_get_bg (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return (vga_con->attribs >> 4) & 0xF;
}

static loom_error
loom_vga_set_x (loom_console *con, loom_usize x)
{
  if (x >= COLS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->x = x;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_y (loom_console *con, loom_usize y)
{
  if (y >= ROWS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->y = y;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_fg (loom_console *con, loom_u8 fg)
{
  if (fg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (vga_con->attribs & 0xF0) | fg;

  return LOOM_ERR_NONE;
}

static loom_error
loom_vga_set_bg (loom_console *con, loom_u8 bg)
{
  if (bg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (vga_con->attribs & 0xF) | (bg << 4);

  return LOOM_ERR_NONE;
}

static void
loom_vga_clear (loom_console *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  loom_u16 char_and_attribs
      = (loom_u16) ' ' | ((loom_u16) vga_con->attribs << 8);

  for (loom_usize i = 0; i < ROWS * COLS; ++i)
    VMEM[i] = char_and_attribs;

  vga_con->x = 0;
  vga_con->y = 0;
  loom_vga_sync_cursor (0, 0);
}

static void
loom_vga_scroll (struct loom_console *con)
{
  (void) con;
}

static loom_usize
loom_vga_write_wbuf (loom_console *con, loom_write_buffer wbuf,
                     loom_u16 attribs, loom_usize index)
{
  for (loom_usize windex = 0; windex < wbuf.len; ++windex)
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

      VMEM[index++] = (loom_u16) ch | attribs;

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
  loom_usize index = vga_con->y * COLS + vga_con->x;
  loom_u16 attribs = (loom_u16) vga_con->attribs << 8;
  loom_write_buffer wbuf;

  if (vga_con->x >= COLS || vga_con->y >= ROWS)
    loom_panic ("loom_vga_write_all");

  for (int i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];

      for (loom_usize j = 0; j < wbuf.splats; ++j)
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
    .interface = {
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
        .next = NULL,
    },
};

void
loom_vga_con_register (void)
{
  loom_con_register (&vga_con.interface);
}