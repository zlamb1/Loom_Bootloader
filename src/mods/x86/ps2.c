#include "loom/compiler.h"
#include "loom/input.h"
#include "loom/keycode.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/platform.h"
#include "loom/platform/x86/io.h"
#include "loom/platform/x86/pic.h"

#define CAP 512

#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_CMD    0x65

loom_compile_assert ((CAP & (CAP - 1)) == 0 && CAP != 0,
                     "CAP must be a power of 2.");

LOOM_MOD (ps2)

typedef struct
{
  loom_input_source_t interface;
  loom_uint16_t poll;
  loom_uint8_t lastkey;
  volatile loom_usize_t head, tail;
  volatile char *buf;
} loom_ps2_keyboard_t;

static int sc1_to_kc[128] = {
  [0x01] = LOOM_KEY_ESCAPE,     [0x02] = LOOM_KEY_1,
  [0x03] = LOOM_KEY_2,          [0x04] = LOOM_KEY_3,
  [0x05] = LOOM_KEY_4,          [0x06] = LOOM_KEY_5,
  [0x07] = LOOM_KEY_6,          [0x08] = LOOM_KEY_7,
  [0x09] = LOOM_KEY_8,          [0x0A] = LOOM_KEY_9,
  [0x0B] = LOOM_KEY_0,          [0x0C] = LOOM_KEY_MINUS,
  [0x0D] = LOOM_KEY_EQUAL,      [0x0E] = LOOM_KEY_BACKSPACE,
  [0x0F] = LOOM_KEY_TAB,        [0x10] = LOOM_KEY_Q,
  [0x11] = LOOM_KEY_W,          [0x12] = LOOM_KEY_E,
  [0x13] = LOOM_KEY_R,          [0x14] = LOOM_KEY_T,
  [0x15] = LOOM_KEY_Y,          [0x16] = LOOM_KEY_U,
  [0x17] = LOOM_KEY_I,          [0x18] = LOOM_KEY_O,
  [0x19] = LOOM_KEY_P,          [0x1A] = LOOM_KEY_LEFTBRACE,
  [0x1B] = LOOM_KEY_RIGHTBRACE, [0x1C] = LOOM_KEY_ENTER,
  [0x1D] = LOOM_KEY_LEFTCTRL,   [0x1E] = LOOM_KEY_A,
  [0x1F] = LOOM_KEY_S,          [0x20] = LOOM_KEY_D,
  [0x21] = LOOM_KEY_F,          [0x22] = LOOM_KEY_G,
  [0x23] = LOOM_KEY_H,          [0x24] = LOOM_KEY_J,
  [0x25] = LOOM_KEY_K,          [0x26] = LOOM_KEY_L,
  [0x27] = LOOM_KEY_SEMICOLON,  [0x28] = LOOM_KEY_APOSTROPHE,
  [0x29] = LOOM_KEY_TILDE,      [0x2A] = LOOM_KEY_LEFTSHIFT,
  [0x2B] = LOOM_KEY_BACKSLASH,  [0x2C] = LOOM_KEY_Z,
  [0x2D] = LOOM_KEY_X,          [0x2E] = LOOM_KEY_C,
  [0x2F] = LOOM_KEY_V,          [0x30] = LOOM_KEY_B,
  [0x31] = LOOM_KEY_N,          [0x32] = LOOM_KEY_M,
  [0x33] = LOOM_KEY_COMMA,      [0x34] = LOOM_KEY_PERIOD,
  [0x35] = LOOM_KEY_SLASH,      [0x36] = LOOM_KEY_RIGHTSHIFT,
  [0x37] = LOOM_KEY_NPASTERISK, [0x38] = LOOM_KEY_LEFTALT,
  [0x39] = LOOM_KEY_SPACE,      [0x3A] = LOOM_KEY_CAPSLOCK,
  [0x3B] = LOOM_KEY_F1,         [0x3C] = LOOM_KEY_F2,
  [0x3D] = LOOM_KEY_F3,         [0x3E] = LOOM_KEY_F4,
  [0x3F] = LOOM_KEY_F5,         [0x40] = LOOM_KEY_F6,
  [0x41] = LOOM_KEY_F7,         [0x42] = LOOM_KEY_F8,
  [0x43] = LOOM_KEY_F9,         [0x44] = LOOM_KEY_F10,
  [0x45] = LOOM_KEY_NUMLOCK,    [0x46] = LOOM_KEY_SCROLLLOCK,
  [0x47] = LOOM_KEY_NP7,        [0x48] = LOOM_KEY_NP8,
  [0x49] = LOOM_KEY_NP9,        [0x4A] = LOOM_KEY_NPMINUS,
  [0x4B] = LOOM_KEY_NP4,        [0x4C] = LOOM_KEY_NP5,
  [0x4D] = LOOM_KEY_NP6,        [0x4E] = LOOM_KEY_NPPLUS,
  [0x4F] = LOOM_KEY_NP1,        [0x50] = LOOM_KEY_NP2,
  [0x51] = LOOM_KEY_NP3,        [0x52] = LOOM_KEY_NP0,
  [0x53] = LOOM_KEY_NPPERIOD,   [0x57] = LOOM_KEY_F11,
  [0x58] = LOOM_KEY_F12,
};

static int sc1_e0_to_kc[128] = {
  [0x1C] = LOOM_KEY_NPENTER, [0x1D] = LOOM_KEY_RIGHTCTRL,
  [0x35] = LOOM_KEY_NPSLASH, [0x38] = LOOM_KEY_RIGHTALT,
  [0x47] = LOOM_KEY_HOME,    [0x48] = LOOM_KEY_UP,
  [0x49] = LOOM_KEY_PAGEUP,  [0x4B] = LOOM_KEY_LEFT,
  [0x4D] = LOOM_KEY_RIGHT,   [0x4F] = LOOM_KEY_END,
  [0x50] = LOOM_KEY_DOWN,    [0x51] = LOOM_KEY_PAGEDOWN,
  [0x52] = LOOM_KEY_INSERT,  [0x53] = LOOM_KEY_DELETE,
};

static loom_ps2_keyboard_t kb = { 0 };

static void
ps2_isr (LOOM_UNUSED loom_uint32_t intno, LOOM_UNUSED loom_uint32_t error_code)
{
  if (kb.buf)
    {
      char ch = (char) loom_inb (PS2_DATA);

      loom_usize_t tail = kb.tail;
      loom_usize_t next_tail = (tail + 1) & (CAP - 1);

      if (kb.head == next_tail)
        goto done;

      kb.buf[tail] = ch;
      kb.tail = next_tail;
    }

done:
  loom_pic_eoi (1);
}

static int
ps2_poll (loom_input_source_t *src, loom_input_event_t *evt)
{
  loom_ps2_keyboard_t *ps2 = (loom_ps2_keyboard_t *) src->data;
  loom_usize_t head, tail;

  int keycode = 0, press;
  loom_uint8_t sc;

  int flags = loom_irq_save ();

  head = ps2->head;
  tail = ps2->tail;

  if (head == tail)
    {
      if (++ps2->poll >= 1000)
        {
          loom_uint8_t status = loom_inb (PS2_STATUS);
          ps2->poll = 0;

          if (status & 1)
            {
              ps2->buf[ps2->tail] = (char) loom_inb (PS2_DATA);
              ps2->tail = (ps2->tail + 1) & (CAP - 1);
            }
        }

      loom_irq_restore (flags);
      return 0;
    }

  ps2->poll = 0;

  sc = (loom_uint8_t) ps2->buf[head];
  press = (sc & 0x80) == 0;

  if (sc != 0xE0)
    sc = (loom_uint8_t) (sc & ~0x80);

  ps2->head = (head + 1) & (CAP - 1);

  loom_irq_restore (flags);

  if (ps2->lastkey == 0xE0)
    {
      keycode = sc1_e0_to_kc[sc];
      ps2->lastkey = 0;
      goto done;
    }

  ps2->lastkey = sc;

  if (sc == 0xE0)
    return 0;

  keycode = sc1_to_kc[sc];

done:
  if (!keycode)
    return 0;

  evt->press = press;
  evt->keycode = keycode;
  evt->mods = 0;

  return 1;
}

LOOM_MOD_INIT ()
{
  kb.interface.poll = ps2_poll;
  kb.interface.data = &kb;

  kb.head = 0;
  kb.tail = 0;
  kb.buf = loom_malloc (CAP);

  if (!kb.buf)
    return;

  loom_input_source_register (&kb.interface);

  loom_pic_register_isr (1, ps2_isr);
  loom_pic_unmask (1);
}

LOOM_MOD_DEINIT ()
{
  loom_pic_mask (1);

  loom_input_source_unregister (&kb.interface);
}