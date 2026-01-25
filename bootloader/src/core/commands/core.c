#include "loom/arch.h"
#include "loom/command.h"
#include "loom/console.h"
#include "loom/kernel_loader.h"
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

static int
rmmod_task (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  if (argc <= 1)
    {
      loom_error (LOOM_ERR_BAD_ARG, "expected module name");
      return -1;
    }

  if (!loom_module_remove (argv[1]))
    {
      loom_error (LOOM_ERR_BAD_ARG, "module %s not found", argv[1]);
      return -1;
    }

  return 0;
}

static int
lsmod_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
            UNUSED char *argv[])
{
  LOOM_LIST_ITERATE (loom_modules, mod) { loom_printf ("%s\n", mod->name); }
  return 0;
}

static int
reboot_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
             UNUSED char *argv[])
{
  loom_arch_reboot ();
  return 0;
}

static int
mmap_print_hook (loom_mmap_entry_t *entry, UNUSED void *data)
{
  loom_printf ("%-#12llx%-#12llx%-12s\n", entry->address, entry->length,
               loom_memory_type_str (entry->type));
  return 0;
}

static int
mmap_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
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

  return 0;
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

static int
fg_task (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_FG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_error (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_fg (console, color);
  }

  return 0;
}

static int
bg_task (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
{
  loom_console_color_t color = LOOM_CONSOLE_DEFAULT_BG;

  if (argc > 1 && !parse_console_color (argv[1], &color))
    {
      loom_error (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  LOOM_LIST_ITERATE (loom_consoles, console)
  {
    console->set_bg (console, color);
  }

  return 0;
}

static int
clear_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
            UNUSED char *argv[])
{
  LOOM_LIST_ITERATE (loom_consoles, console) { console->clear (console); }
  return 0;
}

static int
memory_task (UNUSED loom_command_t *cmd, loom_usize_t argc, char *argv[])
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

  return 0;
}

static int
boot_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
           UNUSED char *argv[])
{
  loom_kernel_loader_boot ();
  loom_error (LOOM_ERR_BAD_ARG, "no kernel loaded");
  return -1;
}

static void
command_register (const char *name, loom_task_t task)
{
  loom_command_t *cmd = loom_malloc (sizeof (loom_command_t));

  if (!cmd)
    return;

  cmd->name = name;
  cmd->task = task;

  loom_command_register (cmd);
}

void
loom_init_core_cmds (void)
{
  command_register ("rmmod", rmmod_task);
  command_register ("lsmod", lsmod_task);
  command_register ("reboot", reboot_task);
  command_register ("mmap", mmap_task);
  command_register ("fg", fg_task);
  command_register ("bg", bg_task);
  command_register ("clear", clear_task);
  command_register ("memory", memory_task);
  command_register ("boot", boot_task);
}