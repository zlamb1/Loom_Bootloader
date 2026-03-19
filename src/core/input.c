#include "loom/input.h"
#include "loom/assert.h"
#include "loom/keycode.h"
#include "loom/list.h"
#include "loom/types.h"

loom_list loom_input_sources = LOOM_LIST_HEAD (loom_input_sources);

void
loomInputSourceRegister (loom_input_source *input_src)
{
  loomAssert (input_src != NULL);
  loomListAdd (&loom_input_sources, &input_src->node);
}

void
loomInputSourceUnregister (loom_input_source *input_src)
{
  loomAssert (input_src != NULL);
  loomListRemove (&input_src->node);
}

int
loomInputSourcePoll (loom_input_source *input_src, loom_input_event *evt)
{
  loomAssert (input_src != NULL);
  loomAssert (input_src->poll != NULL);
  loomAssert (evt != NULL);

  if (input_src->poll (input_src, evt))
    {
      int mask;

      if (evt->mods & LOOM_INPUT_MOD_PASSTHROUGH)
        goto out;

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
      evt->mods |= input_src->mods;

      return 1;
    }

  return 0;
}

int
loomInputSourcesPoll (loom_input_event *evt)
{
  loom_input_source *input_src;

  loom_list_for_each_entry (&loom_input_sources, input_src, node)
  {
    int retval;
    if ((retval = loomInputSourcePoll (input_src, evt)))
      return retval;
  }

  return 0;
}