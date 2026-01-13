#include "loom/shell.h"
#include "loom/input.h"
#include "loom/keycode.h"
#include "loom/print.h"

void
loom_exec_shell (void)
{
  loom_printf ("loom> ");

  loom_input_dev_t *dev = loom_get_root_input_dev ();

  for (;;)
    {
      loom_input_t input;

      if (dev && loom_input_dev_read (dev, &input))
        {
          char ch;

          if (!input.press)
            continue;

          ch = loom_keycode_to_char (dev->mods, input.keycode);

          if (!ch)
            continue;

          loom_printf ("%c", ch);
        }
    }
}