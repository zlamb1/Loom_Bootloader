#include "loom/shell.h"
#include "loom/error.h"
#include "loom/input.h"
#include "loom/keycode.h"
#include "loom/mm.h"
#include "loom/print.h"

#define CAP 4096

typedef struct
{
  loom_usize_t len, cursor;
  char *buf;
} shell_t;

void
shell_write_keycode (shell_t *shell, int mods, int keycode)
{
  switch (keycode)
    {
    case LOOM_KEY_BACKSPACE:
      if (!shell->cursor)
        return;

      if (shell->cursor-- == shell->len--)
        {
          shell->buf[shell->len] = 0;
          loom_printf ("\b \b");
          return;
        }

      loom_printf ("\b%s \b", shell->buf + shell->cursor + 1);
      for (loom_usize_t i = shell->cursor; i < shell->len; ++i)
        {
          shell->buf[i] = shell->buf[i + 1];
          loom_printf ("\b");
        }

      shell->buf[shell->len] = 0;

      break;
    case LOOM_KEY_ENTER:
      shell->len = 0;
      shell->cursor = 0;
      loom_printf ("\nloom> ");
      break;
    case LOOM_KEY_LEFT:
      if (shell->cursor)
        {
          --shell->cursor;
          loom_printf ("\b");
        }
      break;
    case LOOM_KEY_RIGHT:
      if (shell->cursor < shell->len)
        loom_printf ("%c", shell->buf[shell->cursor++]);
      break;
    default:
      {
        char ch;

        if (shell->len >= CAP - 2)
          return;

        ch = loom_keycode_to_char (mods, keycode);

        if (!ch)
          return;

        if (shell->cursor == shell->len)
          {
            shell->cursor++;
            shell->buf[shell->len++] = ch;
            shell->buf[shell->len + 1] = 0;
          }
        else
          shell->buf[shell->cursor] = ch;

        loom_printf ("%c", ch);
        break;
      }
    }
}

void
loom_exec_shell (void)
{
  loom_printf ("loom> ");

  loom_input_dev_t *dev = loom_get_root_input_dev ();

  shell_t shell = { 0 };
  shell.buf = loom_malloc (CAP);
  if (!shell.buf)
    loom_panic ("Out of memory.");

  for (;;)
    {
      loom_input_t input;

      if (dev && loom_input_dev_read (dev, &input))
        {
          if (!input.press)
            continue;

          shell_write_keycode (&shell, dev->mods, input.keycode);
        }
    }
}