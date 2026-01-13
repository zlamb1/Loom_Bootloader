#include "loom/keycode.h"
#include "loom/input.h"

char keycode_to_ascii[128] = {
  [LOOM_KEY_TILDE] = '`',       [LOOM_KEY_1] = '1',
  [LOOM_KEY_2] = '2',           [LOOM_KEY_3] = '3',
  [LOOM_KEY_4] = '4',           [LOOM_KEY_5] = '5',
  [LOOM_KEY_6] = '6',           [LOOM_KEY_7] = '7',
  [LOOM_KEY_8] = '8',           [LOOM_KEY_9] = '9',
  [LOOM_KEY_0] = '0',           [LOOM_KEY_MINUS] = '-',
  [LOOM_KEY_EQUAL] = '=',       [LOOM_KEY_BACKSPACE] = '\b',
  [LOOM_KEY_TAB] = '\t',        [LOOM_KEY_Q] = 'q',
  [LOOM_KEY_W] = 'w',           [LOOM_KEY_E] = 'e',
  [LOOM_KEY_R] = 'r',           [LOOM_KEY_T] = 't',
  [LOOM_KEY_Y] = 'y',           [LOOM_KEY_U] = 'u',
  [LOOM_KEY_I] = 'i',           [LOOM_KEY_O] = 'o',
  [LOOM_KEY_P] = 'p',           [LOOM_KEY_LEFTBRACE] = '[',
  [LOOM_KEY_RIGHTBRACE] = ']',  [LOOM_KEY_BACKSLASH] = '\\',
  [LOOM_KEY_A] = 'a',           [LOOM_KEY_S] = 's',
  [LOOM_KEY_D] = 'd',           [LOOM_KEY_F] = 'f',
  [LOOM_KEY_G] = 'g',           [LOOM_KEY_H] = 'h',
  [LOOM_KEY_J] = 'j',           [LOOM_KEY_K] = 'k',
  [LOOM_KEY_L] = 'l',           [LOOM_KEY_SEMICOLON] = ';',
  [LOOM_KEY_APOSTROPHE] = '\'', [LOOM_KEY_ENTER] = '\n',
  [LOOM_KEY_Z] = 'z',           [LOOM_KEY_X] = 'x',
  [LOOM_KEY_C] = 'c',           [LOOM_KEY_V] = 'v',
  [LOOM_KEY_B] = 'b',           [LOOM_KEY_N] = 'n',
  [LOOM_KEY_M] = 'm',           [LOOM_KEY_COMMA] = ',',
  [LOOM_KEY_PERIOD] = '.',      [LOOM_KEY_SLASH] = '/',
  [LOOM_KEY_SPACE] = ' ',
};

char keycode_to_ascii_alt[128] = {
  [LOOM_KEY_TILDE] = '~',      [LOOM_KEY_1] = '!',
  [LOOM_KEY_2] = '@',          [LOOM_KEY_3] = '#',
  [LOOM_KEY_4] = '$',          [LOOM_KEY_5] = '%',
  [LOOM_KEY_6] = '^',          [LOOM_KEY_7] = '&',
  [LOOM_KEY_8] = '*',          [LOOM_KEY_9] = '(',
  [LOOM_KEY_0] = ')',          [LOOM_KEY_MINUS] = '_',
  [LOOM_KEY_EQUAL] = '+',      [LOOM_KEY_Q] = 'Q',
  [LOOM_KEY_W] = 'W',          [LOOM_KEY_E] = 'E',
  [LOOM_KEY_R] = 'R',          [LOOM_KEY_T] = 'T',
  [LOOM_KEY_Y] = 'Y',          [LOOM_KEY_U] = 'U',
  [LOOM_KEY_I] = 'I',          [LOOM_KEY_O] = 'O',
  [LOOM_KEY_P] = 'P',          [LOOM_KEY_LEFTBRACE] = '{',
  [LOOM_KEY_RIGHTBRACE] = '}', [LOOM_KEY_BACKSLASH] = '|',
  [LOOM_KEY_A] = 'A',          [LOOM_KEY_S] = 'S',
  [LOOM_KEY_D] = 'D',          [LOOM_KEY_F] = 'F',
  [LOOM_KEY_G] = 'G',          [LOOM_KEY_H] = 'H',
  [LOOM_KEY_J] = 'J',          [LOOM_KEY_K] = 'K',
  [LOOM_KEY_L] = 'L',          [LOOM_KEY_SEMICOLON] = ':',
  [LOOM_KEY_APOSTROPHE] = '"', [LOOM_KEY_Z] = 'Z',
  [LOOM_KEY_X] = 'X',          [LOOM_KEY_C] = 'C',
  [LOOM_KEY_V] = 'V',          [LOOM_KEY_B] = 'B',
  [LOOM_KEY_N] = 'N',          [LOOM_KEY_M] = 'M',
  [LOOM_KEY_COMMA] = '<',      [LOOM_KEY_PERIOD] = '>',
  [LOOM_KEY_SLASH] = '?',
};

char
loom_keycode_to_char (int mods, int keycode)
{
  int buf = 0;

  if (loom_keycode_alpha (keycode) || loom_keycode_numeric (keycode)
      || keycode == LOOM_KEY_TILDE || keycode == LOOM_KEY_MINUS
      || keycode == LOOM_KEY_EQUAL || keycode == LOOM_KEY_LEFTBRACE
      || keycode == LOOM_KEY_RIGHTBRACE || keycode == LOOM_KEY_BACKSLASH
      || keycode == LOOM_KEY_SEMICOLON || keycode == LOOM_KEY_APOSTROPHE
      || keycode == LOOM_KEY_COMMA || keycode == LOOM_KEY_PERIOD
      || keycode == LOOM_KEY_SLASH)
    {
      if (mods & LOOM_INPUT_MOD_LEFTSHIFT || mods & LOOM_INPUT_MOD_RIGHTSHIFT)
        buf = (buf + 1) % 2;

      if (loom_keycode_alpha (keycode) && mods & LOOM_INPUT_MOD_CAPSLOCK)
        buf = (buf + 1) % 2;
    }

  return !buf ? keycode_to_ascii[keycode] : keycode_to_ascii_alt[keycode];
}
