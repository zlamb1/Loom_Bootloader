#include "loom/input.h"
#include "loom/error.h"
#include "loom/keycode.h"
#include "loom/list.h"
#include "loom/types.h"

loom_input_source_t *loom_input_sources;

void
loom_input_source_register (loom_input_source_t *src)
{
  src->prev = NULL;
  src->next = loom_input_sources;

  if (loom_input_sources)
    loom_input_sources->prev = src;

  loom_input_sources = src;
}

void
loom_input_source_unregister (loom_input_source_t *src)
{
  if (src->prev)
    src->prev = src->next;
  else
    {
      if (loom_input_sources != src)
        loom_panic ("loom_input_source_unregister");

      loom_input_sources = src->next;
    }

  if (src->next)
    src->next->prev = src->prev;
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

          goto out;
        }
      else
        goto out;

      if (evt->press)
        src->mods |= mask;
      else
        src->mods &= ~mask;

    out:
      evt->mods = src->mods;

      return 1;
    }

  return 0;
}

int
loom_input_sources_read (loom_input_event_t *evt)
{
  LOOM_LIST_ITERATE (loom_input_sources, src)
  {
    int retval;
    if ((retval = loom_input_source_read (src, evt)))
      return retval;
  }

  return 0;
}