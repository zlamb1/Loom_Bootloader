#include "loom/console.h"
#include "loom/string.h"

loom_console_t *consoles = NULL;

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
loom_con_register (loom_console_t *con)
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
  loom_console_t *con = consoles;

  while (con != NULL)
    {
      con->clear (con);
      con = con->next;
    }
}

void
loom_con_write (loom_usize_t len, const char *buf)
{
  loom_console_t *con = consoles;

  while (con != NULL)
    {
      con->write_all (con, (loom_write_buffer_t[]) {
                               { .len = len, .splats = 1, .s = buf },
                               { 0 },
                           });
      con = con->next;
    }
}

void
loom_con_write_str (const char *s)
{
  loom_console_t *con = consoles;
  loom_usize_t len = loom_strlen (s);

  while (con != NULL)
    {
      con->write_all (con, (loom_write_buffer_t[]) {
                               { .len = len, .splats = 1, .s = s },
                               { 0 },
                           });
      con = con->next;
    }
}

void
loom_con_write_all (loom_write_buffer_t wbufs[])
{
  loom_console_t *con = consoles;

  while (con != NULL)
    {
      con->write_all (con, wbufs);
      con = con->next;
    }
}