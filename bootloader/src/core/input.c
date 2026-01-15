#include "loom/input.h"
#include "loom/keycode.h"

loom_input_source_t *loom_input_sources;

void
loom_input_source_register (loom_input_source_t *src)
{
  src->next = loom_input_sources;
  loom_input_sources = src;
}

int
loom_input_source_read (loom_input_source_t *src, loom_input_event_t *evt)
{
  if (!src)
    return 0;

  if (src->read (src, evt))
    {
      int mask;

      if (evt->keycode == LOOM_KEY_LEFTSHIFT)
        mask = LOOM_INPUT_MOD_LEFTSHIFT;
      else if (evt->keycode == LOOM_KEY_RIGHTSHIFT)
        mask = LOOM_INPUT_MOD_RIGHTSHIFT;
      else if (evt->keycode == LOOM_KEY_LEFTALT)
        mask = LOOM_INPUT_MOD_LEFTALT;
      else if (evt->keycode == LOOM_KEY_RIGHTALT)
        mask = LOOM_INPUT_MOD_RIGHTALT;
      else if (evt->keycode == LOOM_KEY_CAPSLOCK)
        {
          if (evt->press)
            {
              if (src->mods & LOOM_INPUT_MOD_CAPSLOCK)
                src->mods &= ~LOOM_INPUT_MOD_CAPSLOCK;
              else
                src->mods |= LOOM_INPUT_MOD_CAPSLOCK;
            }

          return 1;
        }
      else
        return 1;

      if (evt->press)
        src->mods |= mask;
      else
        src->mods &= ~mask;

      return 1;
    }

  return 0;
}