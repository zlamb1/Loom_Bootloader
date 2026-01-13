#include "loom/input.h"
#include "loom/keycode.h"

static loom_input_dev_t *root;

void
loom_register_input_dev (loom_input_dev_t *dev)
{
  dev->next = root;
  root = dev;
}

loom_input_dev_t *
loom_get_root_input_dev (void)
{
  return root;
}

int
loom_input_dev_read (loom_input_dev_t *dev, loom_input_t *input)
{
  if (!dev)
    return 0;

  if (dev->read (dev, input))
    {
      int mask;

      if (input->keycode == LOOM_KEY_LEFTSHIFT)
        mask = LOOM_INPUT_MOD_LEFTSHIFT;
      else if (input->keycode == LOOM_KEY_RIGHTSHIFT)
        mask = LOOM_INPUT_MOD_RIGHTSHIFT;
      else if (input->keycode == LOOM_KEY_LEFTALT)
        mask = LOOM_INPUT_MOD_LEFTALT;
      else if (input->keycode == LOOM_KEY_RIGHTALT)
        mask = LOOM_INPUT_MOD_RIGHTALT;
      else if (input->keycode == LOOM_KEY_CAPSLOCK)
        {
          if (input->press)
            {
              if (dev->mods & LOOM_INPUT_MOD_CAPSLOCK)
                dev->mods &= ~LOOM_INPUT_MOD_CAPSLOCK;
              else
                dev->mods |= LOOM_INPUT_MOD_CAPSLOCK;
            }

          return 1;
        }
      else
        return 1;

      if (input->press)
        dev->mods |= mask;
      else
        dev->mods &= ~mask;

      return 1;
    }

  return 0;
}