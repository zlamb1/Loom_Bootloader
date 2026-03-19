#include "loom/console.h"
#include "loom/assert.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/string.h"

loom_list loom_consoles = LOOM_LIST_HEAD (loom_consoles);

void
loomWbufsPrepend (usize cap, loom_write_buffer wbufs[], loom_write_buffer wbuf)
{
  usize i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loomPanic ("loomWbufsPrepend");

  wbufs[i + 1] = (loom_write_buffer) { 0 };

  while (i)
    {
      wbufs[i] = wbufs[i - 1];
      --i;
    }

  wbufs[0] = wbuf;
}

void
loomWbufsAppend (usize cap, loom_write_buffer wbufs[], loom_write_buffer wbuf)
{
  usize i = 0;

  for (; i < cap - 1; ++i)
    if (wbufs[i].s == NULL)
      break;

  if (i == cap - 1)
    loomPanic ("loomWbufsAppend");

  wbufs[i] = wbuf;
  wbufs[i + 1] = (loom_write_buffer) { 0 };
}

usize
loomWbufsCharLen (loom_write_buffer wbufs[])
{
  usize char_len = 0;
  loom_write_buffer *wbuf = wbufs;

  while (wbuf->s)
    {
      usize tmp;

      if (!wbuf->splats)
        goto cont;

      if (loomMul (wbuf->len, wbuf->splats, &tmp))
        return USIZE_MAX;

      if (loomAdd (char_len, tmp, &char_len))
        return USIZE_MAX;

    cont:
      ++wbuf;
    }

  return char_len;
}

void
loomConsoleRegister (loom_console *console)
{
  loomAssert (console != NULL);
  loomAssert (console->set_fg != NULL);
  loomAssert (console->set_bg != NULL);
  loomAssert (console->clear != NULL);

  console->set_fg (console, LOOM_CONSOLE_DEFAULT_FG);
  console->set_bg (console, LOOM_CONSOLE_DEFAULT_BG);
  console->clear (console);

  loomListAdd (&loom_consoles, &console->node);
}

void
loomConsoleUnregister (loom_console *console)
{
  loomAssert (console != NULL);
  loomListRemove (&console->node);
}

void
loomConsolesClear (void)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->clear != NULL);
    console->clear (console);
  }
}

void
loomConsolesWrite (usize len, const char *buf)
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->write_all != NULL);
    console->write_all (console, (loom_write_buffer[]) {
                                     { .len = len, .splats = 1, .s = buf },
                                     { 0 },
                                 });
  }
}

void
loomConsolesWriteStr (const char *s)
{
  usize len = loomStrLength (s);

  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->write_all != NULL);
    console->write_all (console, (loom_write_buffer[]) {
                                     { .len = len, .splats = 1, .s = s },
                                     { 0 },
                                 });
  }
}

void
loomConsolesWriteAll (loom_write_buffer wbufs[])
{
  loom_console *console;

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->write_all != NULL);
    console->write_all (console, wbufs);
  }
}