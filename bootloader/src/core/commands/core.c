#include "loom/arch.h"
#include "loom/command.h"
#include "loom/console.h"
#include "loom/list.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/print.h"
#include "loom/string.h"

typedef struct
{
  const char *key;
  loom_console_color_t color;
} color_map_t;

static void
loom_cmd_rmmod (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  if (argc <= 1)
    {
      loom_printf ("usage: %s [MODULE]\n", argc ? argv[0] : "rmmod");
      return;
    }

  for (loom_usize_t i = 1; i < argc; ++i)
    if (!loom_module_remove (argv[1]))
      loom_printf ("Module %s not found.\n", argv[1]);
}

static void
loom_cmd_lsmod (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
                UNUSED char *argv[])
{
  LOOM_LIST_ITERATE (loom_modules, mod) { loom_printf ("%s\n", mod->name); }
}

static void
loom_cmd_reboot (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
                 UNUSED char *argv[])
{
  loom_arch_reboot ();
}

static int
mmap_print_hook (loom_mmap_entry_t *entry, UNUSED void *data)
{
  loom_printf ("%-#12llx%-#12llx%-12s\n", entry->address, entry->length,
               loom_memory_type_str (entry->type));
  return 0;
}

static void
loom_cmd_mmap (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
               UNUSED char *argv[])
{
  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_fg (console, LOOM_CONSOLE_COLOR_YELLOW);
  }

  loom_printf ("%-12s%-12s%-12s\n", "ADDRESS", "LENGTH", "TYPE");

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_fg (console, LOOM_CONSOLE_DEFAULT_FG);
  }

  if (loom_mmap.count)
    loom_mmap_iterate (mmap_print_hook, NULL);
  else
    loom_printf ("No Entries\n");
}

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
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_FG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_printf ("%s: bad color: '%s'\n", argv[0], argv[1]);
      return;
    }

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_fg (console, color);
  }
}

static void
loom_cmd_bg (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_BG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_printf ("%s: bad color: '%s'\n", argv[0], argv[1]);
      return;
    }

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_bg (console, color);
  }
}

static void
loom_cmd_clear (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
                UNUSED char *argv[])
{
  LOOM_LIST_ITERATE (loom_consoles, console) { console->clear (console); }
}

static void
loom_cmd_memory (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  int free = 1;

  if (argc > 1
      && (loom_streq (argv[1], "0") || loom_streq (argv[1], "a")
          || loom_streq (argv[1], "allocated")))
    free = 0;

  if (free)
    loom_printf ("%lu bytes free\n", loom_mm_bytes_free ());
  else
    loom_printf ("%lu bytes allocated\n", loom_mm_bytes_allocated ());
}

static void
command_register (const char *name, loom_fn_t fn)
{
  loom_command_t *cmd = loom_malloc (sizeof (loom_command_t));

  if (!cmd)
    return;

  cmd->name = name;
  cmd->fn = fn;

  loom_command_register (cmd);
}

void
loom_init_core_cmds (void)
{
  command_register ("rmmod", loom_cmd_rmmod);
  command_register ("lsmod", loom_cmd_lsmod);
  command_register ("reboot", loom_cmd_reboot);
  command_register ("mmap", loom_cmd_mmap);
  command_register ("fg", loom_cmd_fg);
  command_register ("bg", loom_cmd_bg);
  command_register ("clear", loom_cmd_clear);
  command_register ("memory", loom_cmd_memory);
}