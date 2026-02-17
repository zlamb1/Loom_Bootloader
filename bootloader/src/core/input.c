#include "loom/input.h"
#include "loom/assert.h"
#include "loom/keycode.h"
#include "loom/list.h"
#include "loom/types.h"

loom_list_t loom_input_sources = LOOM_LIST_HEAD (loom_input_sources);

void
loom_input_source_register (loom_input_source_t *input_src)
{
  loom_assert (input_src != NULL);
  loom_list_prepend (&loom_input_sources, &input_src->node);
}

void
loom_input_source_unregister (loom_input_source_t *input_src)
{
  loom_assert (input_src != NULL);
  loom_list_remove (&input_src->node);
}

int
loom_input_source_read (loom_input_source_t *input_src,
                        loom_input_event_t *evt)
{
  if (!input_src)
    return 0;

  if (input_src->read (input_src, evt))
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
              if (input_src->mods & LOOM_INPUT_MOD_CAPSLOCK)
                input_src->mods &= ~LOOM_INPUT_MOD_CAPSLOCK;
              else
                input_src->mods |= LOOM_INPUT_MOD_CAPSLOCK;
            }

          goto out;
        }
      else
        goto out;

      if (evt->press)
        input_src->mods |= mask;
      else
        input_src->mods &= ~mask;

    out:
      evt->mods = input_src->mods;

      return 1;
    }

  return 0;
}

int
loom_input_sources_read (loom_input_event_t *evt)
{
  loom_input_source_t *input_src;

  loom_list_for_each_entry (&loom_input_sources, input_src, node)
  {
    int retval;
    if ((retval = loom_input_source_read (input_src, evt)))
      return retval;
  }

  return 0;
}