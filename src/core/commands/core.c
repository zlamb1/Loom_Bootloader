#include "loom/assert.h"
#include "loom/command.h"
#include "loom/console.h"
#include "loom/kernel_loader.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/platform.h"
#include "loom/print.h"
#include "loom/string.h"

typedef struct
{
  const char *key;
  loom_console_color color;
} color_map;

static int
rmmod_task (unused loom_command *cmd, usize argc, char *argv[])
{
  if (argc <= 1)
    {
      loom_fmt_error (LOOM_ERR_BAD_ARG, "expected module name");
      return -1;
    }

  if (!loom_module_remove (argv[1]))
    {
      loom_fmt_error (LOOM_ERR_BAD_ARG, "module %s not found", argv[1]);
      return -1;
    }

  return 0;
}

static int
lsmod_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_module *module;

  loom_list_for_each_entry (&loom_modules, module, node)
  {
    loom_printf ("%s\n", module->name);
  }

  return 0;
}

static int
reboot_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_reboot ();
  return 0;
}

static int
mmap_print_hook (loom_mmap_entry *entry, unused void *data)
{
  loom_printf ("%-#12llx%-#12llx%-12s\n", entry->addr, entry->length,
               loom_memory_type_str (entry->type));
  return 0;
}

static int
mmap_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_consoles_set_fg (LOOM_CONSOLE_COLOR_YELLOW);

  loom_printf ("%-12s%-12s%-12s\n", "ADDRESS", "LENGTH", "TYPE");

  loom_consoles_set_fg (LOOM_CONSOLE_DEFAULT_FG);

  if (loom_mmap.count)
    loom_mmap_iterate (mmap_print_hook, NULL);
  else
    loom_printf ("No Entries\n");

  return 0;
}

static bool
parse_console_color (usize argc, char *argv[], loom_console_color *color)
{
  int c;
  char *color_arg;
  bool bright = false;

  static color_map map[] = {
    { "black", LOOM_CONSOLE_COLOR_BLACK },
    { "red", LOOM_CONSOLE_COLOR_RED },
    { "green", LOOM_CONSOLE_COLOR_GREEN },
    { "yellow", LOOM_CONSOLE_COLOR_YELLOW },
    { "blue", LOOM_CONSOLE_COLOR_BLUE },
    { "magenta", LOOM_CONSOLE_COLOR_MAGENTA },
    { "purple", LOOM_CONSOLE_COLOR_MAGENTA },
    { "cyan", LOOM_CONSOLE_COLOR_CYAN },
    { "white", LOOM_CONSOLE_COLOR_WHITE },
    { "gray", LOOM_CONSOLE_COLOR_WHITE },
  };

  if (argc < 2)
    return 0;

  color_arg = argv[1];

  if (loom_strtoi (color_arg, &c) == LOOM_ERR_NONE)
    {
      if (c < 0 || c > LOOM_CONSOLE_COLOR_MAX)
        return 0;
      *color = (loom_console_color) c;
      return 1;
    }

  loom_strlower (color_arg);

  if (!loom_strcmp (color_arg, "bright") || !loom_strcmp (color_arg, "light"))
    {
      if (argc < 3)
        return 0;

      bright = 1;
      color_arg = argv[2];
      loom_strlower (color_arg);
    }

  for (unsigned int i = 0; i < sizeof (map) / sizeof (*map); ++i)
    {
      if (!loom_strcmp (color_arg, map[i].key))
        {
          *color = bright ? LOOM_CONSOLE_COLOR_BRIGHT (map[i].color)
                          : map[i].color;
          return 1;
        }
    }

  return 0;
}

static int
fg_task (unused loom_command *cmd, usize argc, char *argv[])
{
  loom_console *console;
  loom_console_color color = LOOM_CONSOLE_DEFAULT_FG;

  if (argc > 1 && !parse_console_color (argc, argv, &color))
    {
      loom_fmt_error (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->set_fg != NULL);
    console->set_fg (console, color);
  }

  return 0;
}

static int
bg_task (unused loom_command *cmd, usize argc, char *argv[])
{
  loom_console *console;
  loom_console_color color = LOOM_CONSOLE_DEFAULT_BG;

  if (argc > 1 && !parse_console_color (argc, argv, &color))
    {
      loom_fmt_error (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loom_assert (console->set_bg != NULL);
    console->set_bg (console, color);
  }

  return 0;
}

static int
clear_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_consoles_clear ();
  return 0;
}

typedef struct
{
  bool is_free;
  usize c;
} mm_iterate_context;

static int
mm_iterate_hook (unused address p, usize n, bool isfree, void *data)
{
  mm_iterate_context *ctx = data;

  if (isfree == ctx->is_free && loom_add (ctx->c, n, &ctx->c))
    {
      ctx->c = USIZE_MAX;
      return 1;
    }

  return 0;
}

static int
memory_task (unused loom_command *cmd, usize argc, char *argv[])
{
  mm_iterate_context ctx = { .c = 0, .is_free = 1 };

  if (argc > 1
      && (!loom_strcmp (argv[1], "0") || !loom_strcmp (argv[1], "a")
          || !loom_strcmp (argv[1], "allocated")))
    ctx.is_free = 0;

  loom_mm_iterate (mm_iterate_hook, &ctx);
  loom_printf ("%lu bytes %s\n", (unsigned long) ctx.c,
               ctx.is_free ? "free" : "allocated");

  return 0;
}

static int
boot_task (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_kernel_loader_boot ();
  loom_fmt_error (LOOM_ERR_BAD_ARG, "no kernel loaded");
  return -1;
}

static void
command_register (const char *name, loom_task task)
{
  loom_command *cmd = loom_malloc (sizeof (loom_command));

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