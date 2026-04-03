#include "loom/console.h"
#include "loom/error.h"
#include "loom/input.h"
#include "loom/keycode.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/platform/x86/io.h"
#include "loom/platform/x86/pic.h"
#include "loom/print.h"
#include "loom/types.h"
#include <stdarg.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

#define SERIAL_READ_PORT            0
#define SERIAL_WRITE_PORT           0
#define SERIAL_BAUD_DIVISOR_LO_PORT 0
#define SERIAL_IRQ_ENABLE_PORT      1
#define SERIAL_BAUD_DIVISOR_HI_PORT 1
#define SERIAL_IRQ_IDENT_PORT       2
#define SERIAL_FIFO_CTRL_PORT       2
#define SERIAL_LINE_CTRL_PORT       3
#define SERIAL_MODEM_CTRL_PORT      4
#define SERIAL_LINE_STATUS_PORT     5
#define SERIAL_MODEM_STATUS_PORT    6
#define SERIAL_SCRATCH_PORT         7

typedef struct serial_console
{
  u16 port;
  usize x, y;
  u8 attribs;
  char last;
  loom_console super;
  struct serial_console *next;
} serial_console;

typedef struct serial_input_source
{
  u16 port;
  loom_input_source super;
  struct serial_input_source *next;
} serial_input_source;

static serial_console *serial_consoles = NULL;
static serial_input_source *serial_input_srcs = NULL;

LOOM_MOD (serial)

static inline void
setBaudRate (u16 port, u16 baud_rate)
{
  u8 line_ctrl = loomInByte (port + SERIAL_LINE_CTRL_PORT);

  if (!(line_ctrl & 0x80))
    loomOutByte (port + SERIAL_LINE_CTRL_PORT, line_ctrl | 0x80);

  loomOutByte (port + SERIAL_BAUD_DIVISOR_LO_PORT, (u8) baud_rate);
  loomOutByte (port + SERIAL_BAUD_DIVISOR_HI_PORT, (u8) (baud_rate >> 8));

  loomOutByte (port + SERIAL_LINE_CTRL_PORT, (u8) (line_ctrl & ~0x80));
}

static used void
serialIrq4 ()
{
  if (loomInByte (COM1 + SERIAL_LINE_STATUS_PORT) & 1)
    loomInByte (COM1 + SERIAL_READ_PORT);

  if (loomInByte (COM3 + SERIAL_LINE_STATUS_PORT) & 1)
    loomInByte (COM3 + SERIAL_READ_PORT);

  loomPICAckIrq (4);
}

static void serialWriteAll (loom_console *console, loom_write_buffer wbufs[]);

static void
serialWrite (loom_write_buffer wbufs[], void *data)
{
  serialWriteAll (data, wbufs);
}

static usize printf_func (2, 3)
    serialFormat (serial_console *console, const char *fmt, ...)
{
  usize ret_val;
  va_list args;
  loom_writer w = { .write = serialWrite, .data = &console->super };
  va_start (args, fmt);
  ret_val = loomWriterFormatV (w, fmt, args);
  va_end (args);
  return ret_val;
}

static usize
serialGetX (loom_console *super)
{
  (void) super;
  return 0;
}

static usize
serialGetY (loom_console *super)
{
  (void) super;
  return 0;
}

static u8
serialGetFg (loom_console *super)
{
  (void) super;
  return 0;
}

static u8
serialGetBg (loom_console *super)
{
  (void) super;
  return 0;
}

static loom_error
serialSetX (loom_console *super, usize x)
{
  (void) super;
  (void) x;
  return LOOM_ERR_NONE;
}

static loom_error
serialSetY (loom_console *super, usize y)
{
  (void) super;
  (void) y;
  return LOOM_ERR_NONE;
}

static loom_error
serialSetFg (loom_console *super, u8 fg)
{
  loomAssert (super != NULL);
  loomAssert (super->data != NULL);

  if (fg > 0xF)
    return LOOM_ERR_BAD_ARG;

  serialFormat (super->data, "\033[%um", fg > 0x7 ? fg + 82 : fg + 30);

  return LOOM_ERR_NONE;
}

static loom_error
serialSetBg (loom_console *super, u8 bg)
{
  loomAssert (super != NULL);
  loomAssert (super->data != NULL);

  if (bg > 0xF)
    return LOOM_ERR_BAD_ARG;

  serialFormat (super->data, "\033[%um", bg > 0x7 ? bg + 92 : bg + 40);

  return LOOM_ERR_NONE;
}

static void
serialClear (loom_console *super)
{
  serial_console *console = super->data;
  console->x = console->y = 0;
  serialFormat (console, "\033[2J\033[1;1H");
}

static inline void
serialPutChar (serial_console *console, char ch)
{
  if (ch == '\n' && console->last != '\r')
    serialPutChar (console, '\r');

  while (!(loomInByte (console->port + SERIAL_LINE_STATUS_PORT) & 0x20))
    ;
  loomOutByte (console->port + SERIAL_WRITE_PORT, (u8) ch);

  console->last = ch;
}

static void
serialWriteAll (loom_console *super, loom_write_buffer wbufs[])
{
  serial_console *console = super->data;
  loom_write_buffer wbuf;

  for (usize i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];
      for (usize j = 0; j < wbuf.splats; ++j)
        for (usize k = 0; k < wbuf.len; ++k)
          serialPutChar (console, wbuf.s[k]);
    }
}

#define SHIFT_FLAG (1 << 15)

static u16 ascii_to_keycode_mod[128] = {
  ['`'] = LOOM_KEY_TILDE,
  ['~'] = LOOM_KEY_TILDE | SHIFT_FLAG,
  ['1'] = LOOM_KEY_1,
  ['!'] = LOOM_KEY_1 | SHIFT_FLAG,
  ['2'] = LOOM_KEY_2,
  ['@'] = LOOM_KEY_2 | SHIFT_FLAG,
  ['3'] = LOOM_KEY_3,
  ['#'] = LOOM_KEY_3 | SHIFT_FLAG,
  ['4'] = LOOM_KEY_4,
  ['$'] = LOOM_KEY_4 | SHIFT_FLAG,
  ['5'] = LOOM_KEY_5,
  ['%'] = LOOM_KEY_5 | SHIFT_FLAG,
  ['6'] = LOOM_KEY_6,
  ['^'] = LOOM_KEY_6 | SHIFT_FLAG,
  ['7'] = LOOM_KEY_7,
  ['&'] = LOOM_KEY_7 | SHIFT_FLAG,
  ['8'] = LOOM_KEY_8,
  ['*'] = LOOM_KEY_8 | SHIFT_FLAG,
  ['9'] = LOOM_KEY_9,
  ['('] = LOOM_KEY_9 | SHIFT_FLAG,
  ['0'] = LOOM_KEY_0,
  [')'] = LOOM_KEY_0 | SHIFT_FLAG,
  ['-'] = LOOM_KEY_MINUS,
  ['_'] = LOOM_KEY_MINUS | SHIFT_FLAG,
  ['='] = LOOM_KEY_EQUAL,
  ['+'] = LOOM_KEY_EQUAL | SHIFT_FLAG,
  ['\b'] = LOOM_KEY_BACKSPACE,
  ['\t'] = LOOM_KEY_TAB,
  ['q'] = LOOM_KEY_Q,
  ['Q'] = LOOM_KEY_Q | SHIFT_FLAG,
  ['w'] = LOOM_KEY_W,
  ['W'] = LOOM_KEY_W | SHIFT_FLAG,
  ['e'] = LOOM_KEY_E,
  ['E'] = LOOM_KEY_E | SHIFT_FLAG,
  ['r'] = LOOM_KEY_R,
  ['R'] = LOOM_KEY_R | SHIFT_FLAG,
  ['t'] = LOOM_KEY_T,
  ['T'] = LOOM_KEY_T | SHIFT_FLAG,
  ['y'] = LOOM_KEY_Y,
  ['Y'] = LOOM_KEY_Y | SHIFT_FLAG,
  ['u'] = LOOM_KEY_U,
  ['U'] = LOOM_KEY_U | SHIFT_FLAG,
  ['i'] = LOOM_KEY_I,
  ['I'] = LOOM_KEY_I | SHIFT_FLAG,
  ['o'] = LOOM_KEY_O,
  ['O'] = LOOM_KEY_O | SHIFT_FLAG,
  ['p'] = LOOM_KEY_P,
  ['P'] = LOOM_KEY_P | SHIFT_FLAG,
  ['['] = LOOM_KEY_LEFTBRACE,
  ['{'] = LOOM_KEY_LEFTBRACE | SHIFT_FLAG,
  [']'] = LOOM_KEY_RIGHTBRACE,
  ['}'] = LOOM_KEY_RIGHTBRACE | SHIFT_FLAG,
  ['\\'] = LOOM_KEY_BACKSLASH,
  ['|'] = LOOM_KEY_BACKSLASH | SHIFT_FLAG,
  ['a'] = LOOM_KEY_A,
  ['A'] = LOOM_KEY_A | SHIFT_FLAG,
  ['s'] = LOOM_KEY_S,
  ['S'] = LOOM_KEY_S | SHIFT_FLAG,
  ['d'] = LOOM_KEY_D,
  ['D'] = LOOM_KEY_D | SHIFT_FLAG,
  ['f'] = LOOM_KEY_F,
  ['F'] = LOOM_KEY_F | SHIFT_FLAG,
  ['g'] = LOOM_KEY_G,
  ['G'] = LOOM_KEY_G | SHIFT_FLAG,
  ['h'] = LOOM_KEY_H,
  ['H'] = LOOM_KEY_H | SHIFT_FLAG,
  ['j'] = LOOM_KEY_J,
  ['J'] = LOOM_KEY_J | SHIFT_FLAG,
  ['k'] = LOOM_KEY_K,
  ['K'] = LOOM_KEY_K | SHIFT_FLAG,
  ['l'] = LOOM_KEY_L,
  ['L'] = LOOM_KEY_L | SHIFT_FLAG,
  [';'] = LOOM_KEY_SEMICOLON,
  [':'] = LOOM_KEY_SEMICOLON | SHIFT_FLAG,
  ['\''] = LOOM_KEY_APOSTROPHE,
  ['"'] = LOOM_KEY_APOSTROPHE | SHIFT_FLAG,
  ['\r'] = LOOM_KEY_ENTER,
  ['\n'] = LOOM_KEY_ENTER,
  [' '] = LOOM_KEY_SPACE,
  ['z'] = LOOM_KEY_Z,
  ['Z'] = LOOM_KEY_Z | SHIFT_FLAG,
  ['x'] = LOOM_KEY_X,
  ['X'] = LOOM_KEY_X | SHIFT_FLAG,
  ['c'] = LOOM_KEY_C,
  ['C'] = LOOM_KEY_C | SHIFT_FLAG,
  ['v'] = LOOM_KEY_V,
  ['V'] = LOOM_KEY_V | SHIFT_FLAG,
  ['b'] = LOOM_KEY_B,
  ['B'] = LOOM_KEY_B | SHIFT_FLAG,
  ['n'] = LOOM_KEY_N,
  ['N'] = LOOM_KEY_N | SHIFT_FLAG,
  ['m'] = LOOM_KEY_M,
  ['M'] = LOOM_KEY_M | SHIFT_FLAG,
  [','] = LOOM_KEY_COMMA,
  ['<'] = LOOM_KEY_COMMA | SHIFT_FLAG,
  ['.'] = LOOM_KEY_PERIOD,
  ['>'] = LOOM_KEY_PERIOD | SHIFT_FLAG,
  ['/'] = LOOM_KEY_SLASH,
  ['?'] = LOOM_KEY_SLASH | SHIFT_FLAG,
  [127] = LOOM_KEY_BACKSPACE,
};

static int
serialPoll (loom_input_source *super, loom_input_event *evt)
{
  serial_input_source *input_src = super->data;
  u16 keycode_mod;
  unsigned char ch;

  if (!(loomInByte (input_src->port + SERIAL_LINE_STATUS_PORT) & 1))
    return 0;

  ch = loomInByte (input_src->port + SERIAL_READ_PORT);

  if (ch & 0x80)
    return 0;

  keycode_mod = ascii_to_keycode_mod[ch];

  if (!keycode_mod)
    return 0;

  evt->press = 1;
  evt->keycode = keycode_mod & ~SHIFT_FLAG;

  if (keycode_mod & SHIFT_FLAG)
    evt->mods = LOOM_INPUT_MOD_LEFTSHIFT | LOOM_INPUT_MOD_PASSTHROUGH;
  else
    evt->mods = 0;

  return 1;
}

LOOM_MOD_INIT ()
{
  static u16 ports[] = { COM1, COM2, COM3, COM4 };

  // Probe serial ports.
  for (uint i = 0; i < sizeof (ports) / sizeof (*ports); ++i)
    {
      u16 port = ports[i];
      u8 save;
      bool did_loopback = false;

      serial_console *console;
      serial_input_source *input_src;

      loomOutByte (port + SERIAL_SCRATCH_PORT, 0xAA);
      if (loomInByte (port + SERIAL_SCRATCH_PORT) != 0xAA)
        continue;

      loomOutByte (port + SERIAL_SCRATCH_PORT, 0x55);
      if (loomInByte (port + SERIAL_SCRATCH_PORT) != 0x55)
        continue;

      save = loomInByte (port + SERIAL_MODEM_CTRL_PORT);

      // Enable loopback mode.
      loomOutByte (port + SERIAL_MODEM_CTRL_PORT, 0b10011);

      loomOutByte (port + SERIAL_WRITE_PORT, 0xAA);
      if (loomInByte (port + SERIAL_READ_PORT) != 0xAA)
        goto restore;

      loomOutByte (port + SERIAL_WRITE_PORT, 0x55);
      if (loomInByte (port + SERIAL_READ_PORT) != 0x55)
        goto restore;

      did_loopback = 1;

    restore:
      // Restore modem control register.
      loomOutByte (port + SERIAL_MODEM_CTRL_PORT, save);

      if (!did_loopback)
        continue;

      // This port is usable.

      console = loomZeroAlloc (sizeof (*console));
      input_src = loomZeroAlloc (sizeof (*input_src));

      if (console == NULL || input_src == NULL)
        {
          // TODO: emit warning
          loomFree (console);
          loomFree (input_src);
          break;
        }

      // For now we disable IRQs. We only use polling writes/read.
      loomOutByte (port + SERIAL_IRQ_ENABLE_PORT, 0);

      // Set baud rate divisor.
      setBaudRate (port, 1);

      // Configure transmit settings to match standard.
      loomOutByte (port + SERIAL_LINE_CTRL_PORT, 0b00000011);

      // Enable OUT2/IRQ and disable loopback mode.
      loomOutByte (port + SERIAL_MODEM_CTRL_PORT, 0b1011);

      console->port = port;
      console->super = (loom_console) {
        .get_x = serialGetX,
        .get_y = serialGetY,
        .get_fg = serialGetFg,
        .get_bg = serialGetBg,
        .set_x = serialSetX,
        .set_y = serialSetY,
        .set_fg = serialSetFg,
        .set_bg = serialSetBg,
        .clear = serialClear,
        .write_all = serialWriteAll,
        .data = console,
      };
      console->next = serial_consoles;

      serial_consoles = console;

      input_src->port = port;
      input_src->super
          = (loom_input_source) { .poll = serialPoll, .data = input_src };
      input_src->next = serial_input_srcs;

      serial_input_srcs = input_src;

      loomConsoleRegister (&console->super);
      loomInputSourceRegister (&input_src->super);
    }
}

LOOM_MOD_DEINIT ()
{
  serial_console *console = serial_consoles, *next_console;
  serial_input_source *input_src = serial_input_srcs, *next_input_src;

  while (console != NULL)
    {
      next_console = console->next;
      loomConsoleUnregister (&console->super);
      loomFree (console);
      console = next_console;
    }

  while (input_src != NULL)
    {
      next_input_src = input_src->next;
      loomInputSourceUnregister (&input_src->super);
      loomFree (input_src);
      input_src = next_input_src;
    }
}