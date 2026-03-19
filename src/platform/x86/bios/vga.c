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
vgaConvertAttribs (u16 attribs)
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
syncCursor (usize x, usize y)
{
  u16 pos = (u16) (y * COLS + x);
  loomOutByte (CRTC_ADDRESS_REG, CRTC_CURSOR_LOW_LOC_REG);
  loomOutByte (CRTC_DATA_REG, (u8) (pos & 0xFF));
  loomOutByte (CRTC_ADDRESS_REG, CRTC_CURSOR_HIGH_LOC_REG);
  loomOutByte (CRTC_DATA_REG, (u8) ((pos >> 8) & 0xFF));
}

static usize
loomVgaGetX (loom_console *super)
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  return console->x;
}

static usize
loomVgaGetY (loom_console *super)
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  return console->y;
}

static u8
loomVgaGetFg (loom_console *super)
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  return console->attribs & 0xF;
}

static u8
loomVgaGetBg (loom_console *super)
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  return (console->attribs >> 4) & 0xF;
}

static loom_error
loomVgaSetX (loom_console *super, usize x)
{
  if (x >= COLS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *console = (loom_vga_console *) super->data;
  console->x = x;

  return LOOM_ERR_NONE;
}

static loom_error
loomVgaSetY (loom_console *super, usize y)
{
  if (y >= ROWS)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *console = (loom_vga_console *) super->data;
  console->y = y;

  return LOOM_ERR_NONE;
}

static loom_error
loomVgaSetFg (loom_console *super, u8 fg)
{
  if (fg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *console = (loom_vga_console *) super->data;
  console->attribs = (console->attribs & 0xF0) | fg;

  return LOOM_ERR_NONE;
}

static loom_error
loomVgaSetBg (loom_console *super, u8 bg)
{
  if (bg > LOOM_CONSOLE_COLOR_MAX)
    return LOOM_ERR_BAD_ARG;

  loom_vga_console *console = (loom_vga_console *) super->data;
  console->attribs = (u8) ((console->attribs & 0xF) | (bg << 4));

  return LOOM_ERR_NONE;
}

static void
loomVgaClear (loom_console *super)
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  u16 char_and_attribs
      = (u16) ' ' | (u16) (vgaConvertAttribs (console->attribs) << 8);

  for (usize i = 0; i < ROWS * COLS; ++i)
    VMEM[i] = char_and_attribs;

  console->x = 0;
  console->y = 0;
  syncCursor (0, 0);
}

static void
loomVgaScroll (loom_console *super)
{
  (void) super;
}

static usize
loomVgaWriteWbuf (loom_console *super, loom_write_buffer wbuf, u16 attribs,
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
            loomPanic ("loom_vga_write_wbuf");

          index = (ROWS - 1) * COLS;
          loomVgaScroll (super);
        }
    }

  return index;
}

static void
loomVgaWriteAll (struct loom_console *super, loom_write_buffer wbufs[])
{
  loom_vga_console *console = (loom_vga_console *) super->data;
  usize index = console->y * COLS + console->x;
  u16 attribs = (u16) (vgaConvertAttribs (console->attribs) << 8);
  loom_write_buffer wbuf;

  if (console->x >= COLS || console->y >= ROWS)
    loomPanic ("loomVgaWriteAll");

  for (int i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];

      for (usize j = 0; j < wbuf.splats; ++j)
        index = loomVgaWriteWbuf (super, wbuf, attribs, index);
    }

  console->x = index % COLS;
  console->y = index / COLS;

  syncCursor (console->x, console->y);
}

static loom_vga_console vga_console = {
    .x = 0,
    .y = 0,
    .attribs = 0,
    .super = {
        .get_x = loomVgaGetX,
        .get_y = loomVgaGetY,
        .get_fg = loomVgaGetFg,
        .get_bg = loomVgaGetBg,
        .set_x = loomVgaSetX,
        .set_y = loomVgaSetY,
        .set_fg = loomVgaSetFg,
        .set_bg = loomVgaSetBg,
        .clear = loomVgaClear,
        .write_all = loomVgaWriteAll,
        .data = &vga_console,
    },
};

void
loomRegisterEarlyVgaConsole (void)
{
  loomConsoleRegister (&vga_console.super);
}