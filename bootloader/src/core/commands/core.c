#include "loom/command.h"
#include "loom/console.h"
#include "loom/mm.h"
#include "loom/print.h"
#include "loom/string.h"

extern loom_console_t *consoles;

typedef struct
{
  const char *key;
  loom_console_color_t color;
} color_map_t;

static loom_bool_t
parse_console_color (char *arg, loom_console_color_t *color)
{
  int c;

  static color_map_t color_map[] = {
    { "black", LOOM_CONSOLE_COLOR_BLACK },
    { "blue", LOOM_CONSOLE_COLOR_BLUE },
    { "green", LOOM_CONSOLE_COLOR_GREEN },
    { "cyan", LOOM_CONSOLE_COLOR_CYAN },
    { "red", LOOM_CONSOLE_COLOR_RED },
    { "purple", LOOM_CONSOLE_COLOR_PURPLE },
    { "brown", LOOM_CONSOLE_COLOR_BROWN },
    { "light gray", LOOM_CONSOLE_COLOR_LIGHT_GRAY },
    { "dark gray", LOOM_CONSOLE_COLOR_DARK_GRAY },
    { "light blue", LOOM_CONSOLE_COLOR_LIGHT_BLUE },
    { "light green", LOOM_CONSOLE_COLOR_LIGHT_GREEN },
    { "light cyan", LOOM_CONSOLE_COLOR_LIGHT_CYAN },
    { "light red", LOOM_CONSOLE_COLOR_LIGHT_RED },
    { "light purple", LOOM_CONSOLE_COLOR_LIGHT_PURPLE },
    { "yellow", LOOM_CONSOLE_COLOR_YELLOW },
    { "white", LOOM_CONSOLE_COLOR_WHITE },
  };

  if (loom_strtoi (arg, &c) == LOOM_ERR_NONE)
    {
      if (c < 0 || c > LOOM_CONSOLE_COLOR_MAX)
        return 0;
      *color = (loom_console_color_t) c;
      return 1;
    }

  loom_strlower (arg);

  for (unsigned int i = 0; i < sizeof (color_map) / sizeof (*color_map); ++i)
    {
      if (loom_streq (arg, color_map[i].key))
        {
          *color = color_map[i].color;
          return 1;
        }
    }

  return 0;
}

static void
loom_cmd_fg (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  loom_console_t *console = consoles;
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_FG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_printf ("%s: bad color: '%s'\n", argv[0], argv[1]);
      return;
    }

  while (console)
    {
      console->set_fg (console, color);
      console = console->next;
    }
}

static void
loom_cmd_bg (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  loom_console_t *console = consoles;
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_BG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_printf ("%s: bad color: '%s'\n", argv[0], argv[1]);
      return;
    }

  while (console)
    {
      console->set_bg (console, color);
      console = console->next;
    }
}

static void
loom_cmd_clear (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
                UNUSED char *argv[])
{
  loom_console_t *console = consoles;

  while (console)
    {
      console->clear (console);
      console = console->next;
    }
}

static void
register_cmd (const char *name, loom_fn_t fn)
{
  loom_command_t *cmd = loom_malloc (sizeof (loom_command_t));

  if (!cmd)
    return;

  cmd->name = name;
  cmd->fn = fn;

  loom_register_command (cmd);
}

void
loom_init_core_cmds (void)
{
  register_cmd ("fg", loom_cmd_fg);
  register_cmd ("bg", loom_cmd_bg);
  register_cmd ("clear", loom_cmd_clear);
}