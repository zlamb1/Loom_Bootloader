#include "loom/console.h"
#include "loom/string.h"

loom_console *consoles = NULL;

void
loom_wbufs_prepend (loom_usize cap, loom_write_buffer wbufs[],
                    loom_write_buffer wbuf)
{
  loom_usize i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loom_panic ("loom_wbufs_prepend");

  wbufs[i + 1] = (loom_write_buffer) { 0 };

  while (i)
    {
      wbufs[i] = wbufs[i - 1];
      --i;
    }

  wbufs[0] = wbuf;
}

void
loom_wbufs_append (loom_usize cap, loom_write_buffer wbufs[],
                   loom_write_buffer wbuf)
{
  loom_usize i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loom_panic ("loom_wbufs_append");

  wbufs[i] = wbuf;
  wbufs[i + 1] = (loom_write_buffer) { 0 };
}

loom_usize
loom_wbufs_char_len (loom_write_buffer wbufs[])
{
  loom_usize char_len = 0;
  loom_write_buffer *wbuf = wbufs;

  while (wbuf->s)
    {
      loom_usize tmp;

      if (!wbuf->splats)
        goto cont;

      if (wbuf->len > LOOM_USIZE_MAX / wbuf->splats)
        return LOOM_USIZE_MAX;

      tmp = wbuf->len * wbuf->splats;
      if (char_len > LOOM_USIZE_MAX - tmp)
        return LOOM_USIZE_MAX;

      char_len += tmp;

    cont:
      ++wbuf;
    }

  return char_len;
}

void
loom_con_register (loom_console *con)
{
  con->set_fg (con, LOOM_CONSOLE_COLOR_WHITE);
  con->set_bg (con, LOOM_CONSOLE_COLOR_BLACK);
  con->clear (con);

  con->next = consoles;
  consoles = con;
}

void
loom_con_clear (void)
{
  loom_console *con = consoles;

  while (con != NULL)
    {
      con->clear (con);
      con = con->next;
    }
}

void
loom_con_write (loom_usize len, const char *buf)
{
  loom_console *con = consoles;

  while (con != NULL)
    {
      con->write_all (con, (loom_write_buffer[]) {
                               { .len = len, .splats = 1, .s = buf },
                               { 0 },
                           });
      con = con->next;
    }
}

void
loom_con_write_str (const char *s)
{
  loom_console *con = consoles;
  loom_usize len = loom_strlen (s);

  while (con != NULL)
    {
      con->write_all (con, (loom_write_buffer[]) {
                               { .len = len, .splats = 1, .s = s },
                               { 0 },
                           });
      con = con->next;
    }
}

void
loom_con_write_all (loom_write_buffer wbufs[])
{
  loom_console *con = consoles;

  while (con != NULL)
    {
      con->write_all (con, wbufs);
      con = con->next;
    }
}