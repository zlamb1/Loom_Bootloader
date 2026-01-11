#ifndef LOOM_MMAP_H
#define LOOM_MMAP_H 1

#include "types.h"

typedef enum
{
  LOOM_MEMORY_TYPE_FREE = 1,
  LOOM_MEMORY_TYPE_ACPI = 2,
  LOOM_MEMORY_TYPE_ACPI_NVS = 3,
  LOOM_MEMORY_TYPE_RESERVED = 4,
  LOOM_MEMORY_TYPE_BAD_RAM = 5
} loom_memory_type_t;

typedef struct
{
  loom_uint64_t address;
  loom_uint64_t length;
  loom_memory_type_t type;
} loom_mmap_entry_t;

#endif