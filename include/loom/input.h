#ifndef LOOM_INPUT_H
#define LOOM_INPUT_H 1

#include "loom/compiler.h"
#include "loom/list.h"

typedef struct
{
  int press;
  int keycode;
  int mods;
} loom_input_event;

typedef struct loom_input_source
{
  int (*poll) (struct loom_input_source *, loom_input_event *);
#define LOOM_INPUT_MOD_LEFTSHIFT   (1 << 1)
#define LOOM_INPUT_MOD_RIGHTSHIFT  (1 << 2)
#define LOOM_INPUT_MOD_LEFTALT     (1 << 3)
#define LOOM_INPUT_MOD_RIGHTALT    (1 << 4)
#define LOOM_INPUT_MOD_CAPSLOCK    (1 << 5)
#define LOOM_INPUT_MOD_PASSTHROUGH (1 << 6)
  int mods;
  void *data;
  loom_list node;
} loom_input_source;

extern loom_list loom_input_sources;

void export (loom_input_source_register) (loom_input_source *input_src);

void export (loom_input_source_unregister) (loom_input_source *input_src);

int export (loom_input_source_poll) (loom_input_source *input_src,
                                     loom_input_event *evt);

int loom_input_sources_poll (loom_input_event *evt);

#endif