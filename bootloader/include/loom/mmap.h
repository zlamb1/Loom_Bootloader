#ifndef LOOM_MMAP_H
#define LOOM_MMAP_H 1

#include "loom/compiler.h"
#include "loom/types.h"

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

typedef struct
{
  loom_usize_t count;
  loom_mmap_entry_t *entries;
} loom_mmap_t;

extern loom_mmap_t loom_mmap;

static LOOM_UNUSED const char *
loom_memory_type_str (loom_memory_type_t type)
{
  if (type == LOOM_MEMORY_TYPE_FREE)
    return "Free";
  else if (type == LOOM_MEMORY_TYPE_ACPI)
    return "ACPI";
  else if (type == LOOM_MEMORY_TYPE_ACPI_NVS)
    return "ACPI NVS";
  else if (type == LOOM_MEMORY_TYPE_BAD_RAM)
    return "Bad Ram";
  else
    return "Reserved";
}

void loom_mmap_init (void);

int LOOM_EXPORT (loom_mmap_iterate) (int (*hook) (loom_mmap_entry_t *entry,
                                                  void *data),
                                     void *data);

#endif