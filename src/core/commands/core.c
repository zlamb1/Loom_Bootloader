#include "loom/commands/core.h"
#include "loom/assert.h"
#include "loom/block_dev.h"
#include "loom/command.h"
#include "loom/compiler.h"
#include "loom/console.h"
#include "loom/crypto/crypto.h"
#include "loom/crypto/md5.h"
#include "loom/crypto/sha1.h"
#include "loom/dir.h"
#include "loom/error.h"
#include "loom/file.h"
#include "loom/fs.h"
#include "loom/kernel_loader.h"
#include "loom/list.h"
#include "loom/math.h"
#include "loom/mm.h"
#include "loom/mmap.h"
#include "loom/module.h"
#include "loom/partition_scheme.h"
#include "loom/platform.h"
#include "loom/print.h"
#include "loom/string.h"

typedef struct
{
  const char *key;
  loom_console_color color;
} color_map;

#define ARGS unused loom_command *cmd, unused usize argc, unused char *argv[]

static int
rmmodTask (ARGS)
{
  if (argc <= 1)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "expected module name");
      return -1;
    }

  if (!loomModuleRemove (argv[1]))
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "module %s not found", argv[1]);
      return -1;
    }

  return 0;
}

static int
lsmodTask (ARGS)
{
  loom_module *module;

  loom_list_for_each_entry (&loom_modules, module, node)
  {
    loomLogLn ("%s", module->name);
  }

  return 0;
}

static int
rebootTask (ARGS)
{
  loomReboot ();
  return 0;
}

static int
mmapPrintHook (loom_mmap_entry *entry, unused void *data)
{
  loomLogLn ("%-#12llx%-#12llx%-12s", entry->addr, entry->length,
             loomMemoryTypeString (entry->type));
  return 0;
}

static int
mmapTask (ARGS)
{
  loomConsolesSetFg (LOOM_CONSOLE_COLOR_YELLOW);

  loomLogLn ("%-12s%-12s%-12s", "ADDRESS", "LENGTH", "TYPE");

  loomConsolesSetFg (LOOM_CONSOLE_DEFAULT_FG);

  if (loom_mmap.count)
    loomMmapIterate (mmapPrintHook, NULL);
  else
    loomLogLn ("No Entries");

  return 0;
}

static bool
parseConsoleColor (usize argc, char *argv[], loom_console_color *color)
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

  if (loomParseInt (color_arg, &c) == LOOM_ERR_NONE)
    {
      if (c < 0 || c > LOOM_CONSOLE_COLOR_MAX)
        return 0;
      *color = (loom_console_color) c;
      return 1;
    }

  loomStrLower (color_arg);

  if (!loomStrCmp (color_arg, "bright") || !loomStrCmp (color_arg, "light"))
    {
      if (argc < 3)
        return 0;

      bright = 1;
      color_arg = argv[2];
      loomStrLower (color_arg);
    }

  for (unsigned int i = 0; i < sizeof (map) / sizeof (*map); ++i)
    {
      if (!loomStrCmp (color_arg, map[i].key))
        {
          *color = bright ? LOOM_CONSOLE_COLOR_BRIGHT (map[i].color)
                          : map[i].color;
          return 1;
        }
    }

  return 0;
}

static int
fgTask (ARGS)
{
  loom_console *console;
  loom_console_color color = LOOM_CONSOLE_DEFAULT_FG;

  if (argc > 1 && !parseConsoleColor (argc, argv, &color))
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_fg != NULL);
    console->set_fg (console, color);
  }

  return 0;
}

static int
bgTask (ARGS)
{
  loom_console *console;
  loom_console_color color = LOOM_CONSOLE_DEFAULT_BG;

  if (argc > 1 && !parseConsoleColor (argc, argv, &color))
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "bad color '%s'", argv[1]);
      return -1;
    }

  loom_list_for_each_entry (&loom_consoles, console, node)
  {
    loomAssert (console->set_bg != NULL);
    console->set_bg (console, color);
  }

  return 0;
}

static int
clearTask (ARGS)
{
  loomConsolesClear ();
  return 0;
}

typedef struct
{
  bool is_free;
  usize c;
} mm_iterate_context;

static int
mmIterateHook (unused address p, usize n, bool is_free, void *data)
{
  mm_iterate_context *ctx = data;

  if (is_free == ctx->is_free && loomAdd (ctx->c, n, &ctx->c))
    {
      ctx->c = USIZE_MAX;
      return 1;
    }

  return 0;
}

static int
memoryTask (ARGS)
{
  mm_iterate_context ctx = { .c = 0, .is_free = 1 };

  if (argc > 1
      && (!loomStrCmp (argv[1], "0") || !loomStrCmp (argv[1], "a")
          || !loomStrCmp (argv[1], "allocated")))
    ctx.is_free = 0;

  loomMMIterate (mmIterateHook, &ctx);
  loomLogLn ("%lu bytes %s", (unsigned long) ctx.c,
             ctx.is_free ? "free" : "allocated");

  return 0;
}

static int
bootTask (ARGS)
{
  loomKernelLoaderBoot ();
  loomErrorFmt (LOOM_ERR_BAD_ARG, "no kernel loaded");
  return -1;
}

static int
searchTask (ARGS)
{
  loom_block_dev *block_dev;
  bool retry = true;

  while (retry)
    {
      retry = false;
      loom_list_for_each_entry (&loom_block_devs, block_dev, node)
      {
        if (!(block_dev->flags & LOOM_BLOCK_DEVICE_FLAG_PROBED))
          retry = true;
        else
          continue;
        loomBlockDevProbe (block_dev, false, true);
      }
    }

  if (loomListIsEmpty (&loom_fs_list))
    {
      loomErrorFmt (LOOM_ERR_NOENT, "no filesystem found");
      return -1;
    }

  loom_prefix_fs = container_of (loom_fs_list.next, loom_fs, node);

  return 0;
}

static int
partSchemesTask (unused loom_command *cmd, unused usize argc,
                 unused char *argv[])
{
  loom_partition_scheme *partition_scheme;

  loom_list_for_each_entry (&loom_partition_schemes, partition_scheme, node)
  {
    loomLogLn ("%s", partition_scheme->name);
  }

  return 0;
}

static int
fsTypesTask (ARGS)
{
  loom_fs_type *fs_type;

  loom_list_for_each_entry (&loom_fs_types, fs_type, node)
  {
    loomLogLn ("%s", fs_type->name);
  }

  return 0;
}

static int
helpTask (ARGS)
{
  loom_command *_cmd;

  loom_list_for_each_entry (&loom_commands, _cmd, node)
  {
    loomLogLn ("%s", _cmd->name);
  }

  return 0;
}

static int
readFile (usize *osize, void **obuf, usize argc, char *argv[])
{
  loom_file file;
  void *buf = null;

  if (loom_prefix_fs == null)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "set prefix to a valid fs");
      return -1;
    }

  if (argc < 2)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "provide a file path");
      return -1;
    }

  if (loomFileOpen (loom_prefix_fs, &file, argv[1]))
    return -1;

  buf = loomAlloc (file.size);
  if (buf == null)
    goto out;

  if (loomFileRead (&file, file.size, buf, null))
    goto out;

  loomFileClose (&file);

  *osize = file.size;
  *obuf = buf;

  return 0;

out:
  loomFileClose (&file);
  loomFree (buf);
  return -1;
}

static int
md5SumTask (ARGS)
{
  usize size;
  void *buf = null;

  if (readFile (&size, &buf, argc, argv))
    return -1;

  loom_digest digest[LOOM_MD5_DIGEST_SIZE];
  loomMD5Hash (size, buf, digest);
  loomPrintHash (LOOM_MD5_DIGEST_SIZE, digest);
  loomLog ("\n");

  loomFree (buf);

  return 0;
}

static int
sha1SumTask (ARGS)
{
  usize size;
  void *buf = null;

  if (readFile (&size, &buf, argc, argv))
    return -1;

  loom_digest digest[LOOM_SHA1_DIGEST_SIZE];
  loomSHA1Hash (size, buf, digest);
  loomPrintHash (LOOM_SHA1_DIGEST_SIZE, digest);
  loomLog ("\n");

  loomFree (buf);

  return 0;
}

static int
catTask (ARGS)
{
  usize size;
  void *buf = null;

  if (readFile (&size, &buf, argc, argv))
    return -1;

  if (size > INT_MAX)
    {
      loomErrorFmt (LOOM_ERR_OVERFLOW, "file too large to display");
      return -1;
    }

  loomLogLn ("%.*s", (int) size, (const char *) buf);

  loomFree (buf);

  return 0;
}

static int
lsTask (ARGS)
{
  loom_dir dir;
  const char *path = "/";

  if (loom_prefix_fs == null)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "set prefix to a valid fs");
      return -1;
    }

  if (argc >= 2)
    path = argv[1];

  if (loomDirOpen (loom_prefix_fs, &dir, path))
    return -1;

  for (;;)
    {
      auto d_entry = loomDirRead (&dir);
      if (d_entry == null)
        {
          if (loom_errno != LOOM_ERR_NONE)
            return -1;
          goto done;
        }

      loomLogLn ("%s", d_entry->name);
    }

done:
  loomDirClose (&dir);

  return 0;
}

static int
headTask (ARGS)
{
  int lines = 10;
  int flag;

  usize file_count;
  bool print_file_name;

  while ((flag = loomGetOpts ("n:", argc, argv)) != -1)
    switch (flag)
      {
      case 'n':
        {
          loom_error error;
          if ((error = loomParseInt (optarg, &lines)))
            {
              loomErrorFmt (error, "bad line count: '%s'", optarg);
              return -1;
            }
        }
      }

  if (loom_prefix_fs == null)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "set prefix to a valid fs");
      return -1;
    }

  file_count = argc - optind;
  print_file_name = file_count > 1;

  if (!file_count)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "provide a file path");
      return -1;
    }

  while (optind < argc)
    {
      const char *path = argv[optind++];
      loom_file file;
      char *buf = null;
      int line_count = 0, offset = 0;

      if (loomFileOpen (loom_prefix_fs, &file, path))
        return -1;

      buf = loomAlloc (file.size);

      if (buf == null)
        {
          loomFileClose (&file);
          return -1;
        }

      if (loomFileRead (&file, file.size, buf, null))
        {
          loomFree (buf);
          loomFileClose (&file);
          return -1;
        }

      if (print_file_name)
        loomLogLn ("==> %s <==", file.name);

      int dir = lines >= 0 ? 1 : -1;
      int start = lines >= 0 ? 0 : (int) file.size - 1;

      if (lines)
        for (offset = start; buf[offset] != '\0'; offset += dir)
          if (buf[offset] == '\n' && ++line_count >= lines)
            break;

      loomLogLn ("%.*s", offset, buf);

      loomFree (buf);
      loomFileClose (&file);
    }

  return 0;
}

static void
registerCommand (const char *name, loom_task task)
{
  loom_command *cmd = loomAlloc (sizeof (loom_command));

  if (!cmd)
    return;

  cmd->name = name;
  cmd->task = task;

  loomCommandRegister (cmd);
}

void
loomCoreCommandsInit (void)
{
  registerCommand ("rmmod", rmmodTask);
  registerCommand ("lsmod", lsmodTask);
  registerCommand ("reboot", rebootTask);
  registerCommand ("mmap", mmapTask);
  registerCommand ("fg", fgTask);
  registerCommand ("bg", bgTask);
  registerCommand ("clear", clearTask);
  registerCommand ("memory", memoryTask);
  registerCommand ("boot", bootTask);
  registerCommand ("search", searchTask);
  registerCommand ("partschemes", partSchemesTask);
  registerCommand ("fstypes", fsTypesTask);
  registerCommand ("help", helpTask);
  registerCommand ("md5sum", md5SumTask);
  registerCommand ("sha1sum", sha1SumTask);
  registerCommand ("cat", catTask);
  registerCommand ("ls", lsTask);
  registerCommand ("head", headTask);
}