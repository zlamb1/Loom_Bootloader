#include "loom/arch/i686/io.h"
#include "loom/console.h"

#define ROWS 25
#define COLS 80

#define VMEM_MAX (ROWS * COLS)

#define CRTC_ADDRESS_REG         0x3D4
#define CRTC_DATA_REG            0x3D5
#define CRTC_CURSOR_HIGH_LOC_REG 0x00E
#define CRTC_CURSOR_LOW_LOC_REG  0x00F

volatile loom_uint16_t *VMEM = (volatile loom_uint16_t *) 0xB8000;

typedef struct
{
  loom_usize_t x, y;
  loom_uint8_t attribs;
  loom_console_t interface;
} loom_vga_console;

static void
loom_vga_sync_cursor (loom_usize_t x, loom_usize_t y)
{
  loom_uint16_t pos = (loom_uint16_t) (y * COLS + x);
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_LOW_LOC_REG);
  loom_outb (CRTC_DATA_REG, (loom_uint8_t) (pos & 0xFF));
  loom_outb (CRTC_ADDRESS_REG, CRTC_CURSOR_HIGH_LOC_REG);
  loom_outb (CRTC_DATA_REG, (loom_uint8_t) ((pos >> 8) & 0xFF));
}

static loom_usize_t
loom_vga_get_x (loom_console_t *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->x;
}

static loom_usize_t
loom_vga_get_y (loom_console_t *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->y;
}

static loom_uint8_t
loom_vga_get_fg (loom_console_t *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return vga_con->attribs & 0xF;
}

static loom_uint8_t
loom_vga_get_bg (loom_console_t *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  return (vga_con->attribs >> 4) & 0xF;
}

static loom_error_t
loom_vga_set_x (loom_console_t *con, loom_usize_t x)
{
  if (x >= COLS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->x = x;

  return LOOM_ERR_NONE;
}

static loom_error_t
loom_vga_set_y (loom_console_t *con, loom_usize_t y)
{
  if (y >= ROWS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->y = y;

  return LOOM_ERR_NONE;
}

static loom_error_t
loom_vga_set_fg (loom_console_t *con, loom_uint8_t fg)
{
  if (fg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (vga_con->attribs & 0xF0) | fg;

  return LOOM_ERR_NONE;
}

static loom_error_t
loom_vga_set_bg (loom_console_t *con, loom_uint8_t bg)
{
  if (bg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  vga_con->attribs = (vga_con->attribs & 0xF) | (bg << 4);

  return LOOM_ERR_NONE;
}

static void
loom_vga_clear (loom_console_t *con)
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  loom_uint16_t char_and_attribs
      = (loom_uint16_t) ' ' | ((loom_uint16_t) vga_con->attribs << 8);

  for (loom_usize_t i = 0; i < ROWS * COLS; ++i)
    VMEM[i] = char_and_attribs;

  vga_con->x = 0;
  vga_con->y = 0;
  loom_vga_sync_cursor (0, 0);
}

static void
loom_vga_scroll (struct loom_console_t *con)
{
  (void) con;
}

static loom_usize_t
loom_vga_write_wbuf (loom_console_t *con, loom_write_buffer_t wbuf,
                     loom_uint16_t attribs, loom_usize_t index)
{
  for (loom_usize_t windex = 0; windex < wbuf.len; ++windex)
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

      VMEM[index++] = (loom_uint16_t) ch | attribs;

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
loom_vga_write_all (struct loom_console_t *con, loom_write_buffer_t wbufs[])
{
  loom_vga_console *vga_con = (loom_vga_console *) con->data;
  loom_usize_t index = vga_con->y * COLS + vga_con->x;
  loom_uint16_t attribs = (loom_uint16_t) vga_con->attribs << 8;
  loom_write_buffer_t wbuf;

  if (vga_con->x >= COLS || vga_con->y >= ROWS)
    loom_panic ("loom_vga_write_all");

  for (int i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];

      for (loom_usize_t j = 0; j < wbuf.splats; ++j)
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