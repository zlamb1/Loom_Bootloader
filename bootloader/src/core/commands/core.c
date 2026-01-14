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
  static color_map_t color_map[] = {
    { "0", LOOM_CONSOLE_COLOR_BLACK },
    { "1", LOOM_CONSOLE_COLOR_BLUE },
    { "2", LOOM_CONSOLE_COLOR_GREEN },
    { "3", LOOM_CONSOLE_COLOR_CYAN },
    { "4", LOOM_CONSOLE_COLOR_RED },
    { "5", LOOM_CONSOLE_COLOR_PURPLE },
    { "6", LOOM_CONSOLE_COLOR_BROWN },
    { "7", LOOM_CONSOLE_COLOR_LIGHT_GRAY },
    { "8", LOOM_CONSOLE_COLOR_DARK_GRAY },
    { "9", LOOM_CONSOLE_COLOR_LIGHT_BLUE },
    { "10", LOOM_CONSOLE_COLOR_LIGHT_GREEN },
    { "11", LOOM_CONSOLE_COLOR_LIGHT_CYAN },
    { "12", LOOM_CONSOLE_COLOR_LIGHT_RED },
    { "13", LOOM_CONSOLE_COLOR_LIGHT_PURPLE },
    { "14", LOOM_CONSOLE_COLOR_YELLOW },
    { "15", LOOM_CONSOLE_COLOR_WHITE },
  };

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

void
loom_init_core_cmds (void)
{
  loom_command_t *fg_cmd = loom_malloc (sizeof (loom_command_t));
  loom_command_t *bg_cmd = loom_malloc (sizeof (loom_command_t));
  loom_command_t *clear_cmd = loom_malloc (sizeof (loom_command_t));

  if (!fg_cmd || !bg_cmd)
    {
      loom_free (fg_cmd);
      loom_free (bg_cmd);
      loom_free (clear_cmd);
      return;
    }

  fg_cmd->name = "fg";
  fg_cmd->fn = loom_cmd_fg;

  bg_cmd->name = "bg";
  bg_cmd->fn = loom_cmd_bg;

  clear_cmd->name = "clear";
  clear_cmd->fn = loom_cmd_clear;

  loom_register_command (fg_cmd);
  loom_register_command (bg_cmd);
  loom_register_command (clear_cmd);
}