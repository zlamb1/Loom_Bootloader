#ifndef LOOM_MODULE_H
#define LOOM_MODULE_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

#ifdef LOOM_MODULE
#define LOOM_MOD(NAME)    const char LOOM_USED *loom_mod_name = #NAME;
#define LOOM_MOD_INIT()   void LOOM_USED loom_mod_init (void)
#define LOOM_MOD_DEINIT() void LOOM_USED loom_mod_deinit (void)
#endif

typedef struct
{
#define LOOM_MODULE_HEADER_MAGIC 0x70D61E9C
  loom_uint32_t magic;
  loom_uint32_t taboff;
  loom_uint32_t modoff;
#define LOOM_MODULE_HEADER_MIN_SIZE sizeof (loom_module_header_t) + 4
  loom_uint32_t size;
  loom_uint32_t kernel_size;
  loom_uint32_t initrdsize;
} LOOM_PACKED loom_module_header_t;

typedef struct loom_module_section_t
{
  loom_usize_t shidx, size;
#ifdef LOOM_DEBUG
#define LOOM_MODULE_SECTION_NAME_LEN 32
  char name[LOOM_MODULE_SECTION_NAME_LEN];
#endif
  void *p;
  struct loom_module_section_t *next;
} loom_module_section_t;

typedef void (*loom_module_init_t) (void);
typedef void (*loom_module_deinit_t) (void);

typedef struct loom_module_t
{
  const char *name;
  loom_module_init_t init;
  loom_module_deinit_t deinit;
  loom_module_section_t *sections;
  loom_list_t node;
} loom_module_t;

extern loom_address_t LOOM_EXPORT_VAR (loom_modbase);
extern loom_address_t LOOM_EXPORT_VAR (loom_modend);

extern loom_list_t LOOM_EXPORT_VAR (loom_modules);

loom_address_t loom_modend_get (void);
void loom_core_modules_load (void);

void LOOM_EXPORT (loom_module_add) (loom_module_t *mod);
loom_bool_t LOOM_EXPORT (loom_module_remove) (const char *name);
void LOOM_EXPORT (loom_module_unload) (loom_module_t *mod);

#endif