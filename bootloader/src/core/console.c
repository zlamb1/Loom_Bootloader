#include "loom/console.h"
#include "loom/assert.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/string.h"

loom_list_t loom_consoles = LOOM_LIST_HEAD (loom_consoles);

void
loom_wbufs_prepend (loom_usize_t cap, loom_write_buffer_t wbufs[],
                    loom_write_buffer_t wbuf)
{
  loom_usize_t i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loom_panic ("loom_wbufs_prepend");

  wbufs[i + 1] = (loom_write_buffer_t) { 0 };

  while (i)
    {
      wbufs[i] = wbufs[i - 1];
      --i;
    }

  wbufs[0] = wbuf;
}

void
loom_wbufs_append (loom_usize_t cap, loom_write_buffer_t wbufs[],
                   loom_write_buffer_t wbuf)
{
  loom_usize_t i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loom_panic ("loom_wbufs_append");

  wbufs[i] = wbuf;
  wbufs[i + 1] = (loom_write_buffer_t) { 0 };
}

loom_usize_t
loom_wbufs_char_len (loom_write_buffer_t wbufs[])
{
  loom_usize_t char_len = 0;
  loom_write_buffer_t *wbuf = wbufs;

  while (wbuf->s)
    {
      loom_usize_t tmp;

      if (!wbuf->splats)
        goto cont;

      if (loom_mul (wbuf->len, wbuf->splats, &tmp))
        return LOOM_USIZE_MAX;

      if (loom_add (char_len, tmp, &char_len))
        return LOOM_USIZE_MAX;

    cont:
      ++wbuf;
    }

  return char_len;
}

void
loom_console_register (loom_console_t *console)
{
  loom_assert (console != NULL);
  loom_assert (console->set_fg != NULL);
  loom_assert (console->set_bg != NULL);
  loom_assert (console->clear != NULL);

  console->set_fg (console, LOOM_CONSOLE_DEFAULT_FG);
  console->set_bg (console, LOOM_CONSOLE_DEFAULT_BG);
  console->clear (console);

  loom_list_prepend (&loom_consoles, &console->node);
}

void
loom_consoles_clear (void)
{
  loom_console_t *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->clear != NULL);
    console->clear (console);
  }
}

void
loom_consoles_write (loom_usize_t len, const char *buf)
{
  loom_console_t *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->write_all != NULL);
    console->write_all (console, (loom_write_buffer_t[]) {
                                     { .len = len, .splats = 1, .s = buf },
                                     { 0 },
                                 });
  }
}

void
loom_consoles_write_str (const char *s)
{
  loom_usize_t len = loom_strlen (s);

  loom_console_t *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->write_all != NULL);
    console->write_all (console, (loom_write_buffer_t[]) {
                                     { .len = len, .splats = 1, .s = s },
                                     { 0 },
                                 });
  }
}

void
loom_consoles_write_all (loom_write_buffer_t wbufs[])
{
  loom_console_t *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->write_all != NULL);
    console->write_all (console, wbufs);
  }
}