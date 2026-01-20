#include "loom/console.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/string.h"

loom_console_t *loom_consoles = NULL;

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
  console->set_fg (console, LOOM_CONSOLE_DEFAULT_FG);
  console->set_bg (console, LOOM_CONSOLE_DEFAULT_BG);
  console->clear (console);

  console->next = loom_consoles;
  loom_consoles = console;
}

void
loom_console_clear (void)
{
  LOOM_LIST_ITERATE (loom_consoles, console) { console->clear (console); }
}

void
loom_console_write (loom_usize_t len, const char *buf)
{
  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->write_all (console, (loom_write_buffer_t[]) {
                                     { .len = len, .splats = 1, .s = buf },
                                     { 0 },
                                 });
  }
}

void
loom_console_write_str (const char *s)
{
  loom_usize_t len = loom_strlen (s);

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->write_all (console, (loom_write_buffer_t[]) {
                                     { .len = len, .splats = 1, .s = s },
                                     { 0 },
                                 });
  }
}

void
loom_console_write_all (loom_write_buffer_t wbufs[])
{
  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->write_all (console, wbufs);
  }
}