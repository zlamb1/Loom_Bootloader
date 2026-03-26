#include "loom/assert.h"
#include "loom/print.h"
#include "loom/types.h"

const char *optarg = null;
usize optind = 1;
char optopt = 0;

static usize ind = 0;
static usize pos = 0;

static void
shiftPositionalArgs (char *argv[])
{
  const char *arg, **iter;

  loomAssert (pos < optind);

  if (!pos)
    return;

  arg = argv[optind];
  iter = (const char **) argv + optind;

  for (usize i = 0; i < pos; ++i, --iter)
    iter[0] = iter[-1];

  *iter = arg;
}

void
loomGetOptsReset (void)
{
  optarg = null;
  optind = 1;
  optopt = 0;
  ind = 0;
  pos = 0;
}

int
loomGetOpts (const char *optstring, usize argc, char *argv[])
{
  optarg = null;
  optopt = 0;

  while (optind < argc)
    {
      const char *arg = argv[optind];
      char flag;

      bool found = false;
      bool has_arg;

      if (!ind)
        {
          if (arg[0] == '-' && arg[1] != '\0')
            {
              if (arg[1] == '-' && arg[2] == '\0')
                {
                  shiftPositionalArgs (argv);
                  pos += argc - (optind + 1);
                  goto done;
                }
              ind++;
              continue;
            }

          pos++;
          optind++;
          continue;
        }

      flag = arg[ind++];

      if (flag == '\0')
        {
          shiftPositionalArgs (argv);
          optind++;
          ind = 0;
          continue;
        }

      for (usize i = 0; optstring[i] != '\0'; i++)
        {
          if (optstring[i] == flag)
            {
              found = true;
              has_arg = optstring[i + 1] == ':';
              break;
            }
        }

      optopt = flag;

      if (!found)
        flag = '?';
      else if (has_arg)
        {
          auto off = ind;

          shiftPositionalArgs (argv);
          optind++;
          ind = 0;

          if (arg[off] != '\0')
            optarg = arg + off;
          else
            {
              if (optind == argc)
                {
                  loomLogLn ("%s: option requires an argument -- '%c'",
                             argv[0], flag);
                  return ':';
                }
              optarg = argv[optind++];
            }
        }

      return flag;
    }

done:
  optind = argc - pos;
  return -1;
}