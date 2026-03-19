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

struct shell
{
  usize len, cursor;
  char *buf;
};

static void
shellPrintPrompt (void)
{
  loomConsolesSetFg (LOOM_CONSOLE_COLOR_BRIGHT (LOOM_CONSOLE_COLOR_CYAN));
  loomLog (PROMPT);
  loomConsolesSetFg (LOOM_CONSOLE_DEFAULT_FG);
}

static char *
shellParseArg (char *buf, usize *pos)
{
  usize runpos = *pos, runlen = 0;

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
shellExecCommand (struct shell *shell)
{
  loom_command *command;
  usize argc = 0, argvc = 0, pos = 0;
  char **argv = NULL, *arg;

  while ((arg = shellParseArg (shell->buf, &pos)))
    {
      if (argc == argvc)
        {
          char **newargv;

          if (!argvc)
            argvc = 1;
          else
            argvc *= 2;

          newargv = loomRealloc (argv, argvc * sizeof (char *));
          if (!newargv)
            {
              loomFree (argv);
              loomPanic ("Out of memory.");
            }

          argv = newargv;
        }

      argv[argc++] = arg;
    }

  if (!argc)
    return;

  command = loomCommandFind (argv[0]);

  if (command)
    {
      if (command->task (command, argc, argv))
        {
          loomLog ("%s: ", argv[0]);
          loomConsolesSaveFg ();
          loomConsolesSetFg (
              LOOM_CONSOLE_COLOR_BRIGHT (LOOM_CONSOLE_COLOR_RED));
          loomLog ("error: ");
          loomConsolesRestoreFg ();
          loomLogLn ("%s", loomErrorGet ());
          loomErrorClear ();
        }
    }
  else
    loomLogLn ("unknown command: '%s'", argv[0]);

  loomFree (argv);
}

static void
shellWriteKeyCode (struct shell *shell, int mods, int keycode)
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
          loomLog ("\b \b");
          return;
        }

      loomLog ("\b%s \b", buf + shell->cursor + 1);
      for (usize i = shell->cursor; i < shell->len; ++i)
        buf[i] = shell->buf[i + 1];

      {
        loom_write_buffer wbufs[]
            = { { .len = 1, .splats = shell->len - shell->cursor, .s = "\b" },
                { 0 } };
        loomConsolesWriteAll (wbufs);
      }

      shell->buf[shell->len] = 0;

      break;
    case LOOM_KEY_ENTER:
      loomLog ("\n");
      shellExecCommand (shell);
      shell->len = 0;
      shell->cursor = 0;
      shell->buf[0] = 0;
      shellPrintPrompt ();
      break;
    case LOOM_KEY_LEFT:
      if (shell->cursor)
        {
          --shell->cursor;
          loomLog ("\b");
        }
      break;
    case LOOM_KEY_RIGHT:
      if (shell->cursor < shell->len)
        loomLog ("%c", shell->buf[shell->cursor++]);
      break;
    default:
      {
        char ch;

        if (shell->len >= CAP - 2)
          return;

        ch = loomKeyCodeToChar (mods, keycode);

        if (!ch || ch == '\t' || ch == '\r')
          return;

        if (shell->cursor == shell->len)
          {
            shell->cursor++;
            buf[shell->len++] = ch;
            buf[shell->len + 1] = 0;
            loomLog ("%c", ch);
          }
        else
          {
            loom_write_buffer wbufs[] = {
              { .len = 1, .splats = shell->len - shell->cursor, .s = "\b" },
              { 0 }
            };

            for (usize i = shell->len - 1; i >= shell->cursor; --i)
              {
                buf[i + 1] = buf[i];
                if (!i)
                  break;
              }

            buf[shell->cursor++] = ch;
            buf[++shell->len] = 0;

            loomLog ("%c%s", ch, buf + shell->cursor);
            loomConsolesWriteAll (wbufs);
          }

        break;
      }
    }
}

void
loomShellExec (void)
{
  struct shell shell = { 0 };
  shell.buf = loomAlloc (CAP);

  if (!shell.buf)
    loomPanic ("Out of memory.");

  shell.buf[0] = 0;

  shellPrintPrompt ();

  for (;;)
    {
      loom_input_event evt;

      if (loomInputSourcesPoll (&evt))
        {
          if (!evt.press)
            continue;

          shellWriteKeyCode (&shell, evt.mods, evt.keycode);
        }
    }
}