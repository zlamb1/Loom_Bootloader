#ifndef LOOM_BUFFER_H
#define LOOM_BUFFER_H 1

#include "loom/types.h"

typedef struct
{
  usize size;
  void *data;
} loom_buffer;

#endif