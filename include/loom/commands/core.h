#ifndef LOOM_COMMANDS_CONSOLE_H
#define LOOM_COMMANDS_CONSOLE_H 1

#include "loom/types.h"

extern const char *optarg;
extern usize optind;
extern char optopt;

void loomCoreCommandsInit (void);

void loomGetOptsReset (void);

int loomGetOpts (const char *optstring, usize argc, char *argv[]);

#endif