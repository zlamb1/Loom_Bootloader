#ifndef LOOM_INPUT_H
#define LOOM_INPUT_H 1

#include "loom/compiler.h"

typedef struct
{
  int press;
  int keycode;
} loom_input_event_t;

typedef struct loom_input_source_t
{
  int (*read) (struct loom_input_source_t *, loom_input_event_t *);
#define LOOM_INPUT_MOD_LEFTSHIFT  (1 << 1)
#define LOOM_INPUT_MOD_RIGHTSHIFT (1 << 2)
#define LOOM_INPUT_MOD_LEFTALT    (1 << 3)
#define LOOM_INPUT_MOD_RIGHTALT   (1 << 4)
#define LOOM_INPUT_MOD_CAPSLOCK   (1 << 5)
  int mods;
  void *data;
  struct loom_input_source_t *next;
} loom_input_source_t;

extern loom_input_source_t *loom_input_sources;

void LOOM_EXPORT (loom_input_source_register) (loom_input_source_t *src);

int LOOM_EXPORT (loom_input_source_read) (loom_input_source_t *src,
                                          loom_input_event_t *evt);

#endif