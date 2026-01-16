#include "loom/shell.h"
#include "loom/command.h"
#include "loom/console.h"
#include "loom/error.h"
#include "loom/input.h"
#include "loom/keycode.h"
#include "loom/mm.h"
#include "loom/print.h"

#define CAP    4096
#define PROMPT "loom> "

typedef struct
{
  loom_usize_t len, cursor;
  char *buf;
} shell_t;

static char *
shell_parse_arg (char *buf, loom_usize_t *pos)
{
  loom_usize_t runpos = *pos, runlen = 0;

  while (buf[runpos])
    {
      if (buf[runpos] == ' ')
        {
          if (runlen)
            break;
          goto next;
        }

      ++runlen;

    next:
      ++runpos;
    }

  if (!runlen)
    return NULL;

  if (buf[runpos])
    {
      buf[runpos] = 0;
      *pos = runpos + 1;
    }
  else
    *pos = runpos;

  return buf + (runpos - runlen);
}

static void
shell_exec_command (shell_t *shell)
{
  loom_command_t *command;
  loom_usize_t argc = 0, argvc = 0, pos = 0;
  char **argv = NULL, *arg;

  while ((arg = shell_parse_arg (shell->buf, &pos)))
    {
      if (argc == argvc)
        {
          char **newargv;
          loom_usize_t oldargvc = argvc;

          if (!argvc)
            argvc = 1;
          else
            argvc *= 2;

          newargv = loom_realloc (argv, oldargvc * sizeof (char *),
                                  argvc * sizeof (char *));
          if (!newargv)
            {
              loom_free (argv);
              loom_panic ("Out of memory.");
            }

          argv = newargv;
        }

      argv[argc++] = arg;
    }

  if (!argc)
    return;

  command = loom_command_find (argv[0]);

  if (command)
    command->fn (command, argc, argv);
  else
    loom_printf ("unknown command: '%s'\n", argv[0]);

  loom_free (argv);
}

static void
shell_write_keycode (shell_t *shell, int mods, int keycode)
{
  char *buf = shell->buf;

  switch (keycode)
    {
    case LOOM_KEY_BACKSPACE:
      if (!shell->cursor)
        return;

      if (shell->cursor-- == shell->len--)
        {
          buf[shell->len] = 0;
          loom_printf ("\b \b");
          return;
        }

      loom_printf ("\b%s \b", buf + shell->cursor + 1);
      for (loom_usize_t i = shell->cursor; i < shell->len; ++i)
        buf[i] = shell->buf[i + 1];

      {
        loom_write_buffer_t wbufs[]
            = { { .len = 1, .splats = shell->len - shell->cursor, .s = "\b" },
                { 0 } };
        loom_console_write_all (wbufs);
      }

      shell->buf[shell->len] = 0;

      break;
    case LOOM_KEY_ENTER:
      loom_printf ("\n");
      shell_exec_command (shell);
      shell->len = 0;
      shell->cursor = 0;
      shell->buf[0] = 0;
      loom_printf (PROMPT);
      break;
    case LOOM_KEY_LEFT:
      if (shell->cursor)
        {
          --shell->cursor;
          loom_printf ("\b");
        }
      break;
    case LOOM_KEY_RIGHT:
      if (shell->cursor < shell->len)
        loom_printf ("%c", shell->buf[shell->cursor++]);
      break;
    default:
      {
        char ch;

        if (shell->len >= CAP - 2)
          return;

        ch = loom_keycode_to_char (mods, keycode);

        if (!ch || ch == '\t' || ch == '\r')
          return;

        if (shell->cursor == shell->len)
          {
            shell->cursor++;
            buf[shell->len++] = ch;
            buf[shell->len + 1] = 0;
            loom_printf ("%c", ch);
          }
        else
          {
            loom_write_buffer_t wbufs[] = {
              { .len = 1, .splats = shell->len - shell->cursor, .s = "\b" },
              { 0 }
            };

            for (loom_usize_t i = shell->len - 1; i >= shell->cursor; --i)
              {
                buf[i + 1] = buf[i];
                if (!i)
                  break;
              }

            buf[shell->cursor++] = ch;
            buf[++shell->len] = 0;

            loom_printf ("%c%s", ch, buf + shell->cursor);
            loom_console_write_all (wbufs);
          }

        break;
      }
    }
}

void
loom_shell_exec (void)
{
  loom_printf (PROMPT);

  loom_input_source_t *src = loom_input_sources;

  shell_t shell = { 0 };
  shell.buf = loom_malloc (CAP);

  if (!shell.buf)
    loom_panic ("Out of memory.");

  shell.buf[0] = 0;

  for (;;)
    {
      loom_input_event_t evt;

      if (src && loom_input_source_read (src, &evt))
        {
          if (!evt.press)
            continue;

          shell_write_keycode (&shell, src->mods, evt.keycode);
        }
    }
}