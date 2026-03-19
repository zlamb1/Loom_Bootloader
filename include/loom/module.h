#ifndef LOOM_MODULE_H
#define LOOM_MODULE_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

#ifdef LOOM_MODULE
#define LOOM_MOD(NAME)    const char used *loom_mod_name = #NAME;
#define LOOM_MOD_INIT()   void used loom_mod_init (void)
#define LOOM_MOD_DEINIT() void used loom_mod_deinit (void)
#endif

typedef struct
{
#define LOOM_MODULE_HEADER_MAGIC 0x70D61E9C
  u32 magic;
  u32 taboff;
  u32 modoff;
#define LOOM_MODULE_HEADER_MIN_SIZE sizeof (loom_module_header) + 4
  u32 size;
  u32 kernel_size;
  u32 initrdsize;
} packed loom_module_header;

typedef struct loom_module_section
{
  usize shidx, size;
#ifdef LOOM_DEBUG
#define LOOM_MODULE_SECTION_NAME_LEN 32
  char name[LOOM_MODULE_SECTION_NAME_LEN];
#endif
  void *p;
  struct loom_module_section *next;
} loom_module_section;

typedef void (*loom_module_init) (void);
typedef void (*loom_module_deinit) (void);

typedef struct loom_module
{
  const char *name;
  loom_module_init init;
  loom_module_deinit deinit;
  loom_module_section *sections;
  loom_list node;
} loom_module;

extern address export_var (loom_modbase);
extern address export_var (loom_modend);

extern loom_list export_var (loom_modules);

address loom_modend_get (void);
void loom_core_modules_load (void);

void export (loom_module_add) (loom_module *mod);
bool export (loom_module_remove) (const char *name);
void export (loom_module_unload) (loom_module *mod);

#endif