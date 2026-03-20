#ifndef LOOM_MODULE_H
#define LOOM_MODULE_H 1

#include "loom/compiler.h"
#include "loom/list.h"
#include "loom/types.h"

#ifdef LOOM_MODULE
#define LOOM_MOD(NAME)    const char used *loom_mod_name = #NAME;
#define LOOM_MOD_INIT()   void used loomModInit (void)
#define LOOM_MOD_DEINIT() void used loomModDeinit (void)
#elif defined(LOOM_UTIL)
#define LOOM_MOD(NAME)
#define LOOM_MOD_INIT()   static unused void loomModInit (void)
#define LOOM_MOD_DEINIT() static unused void loomModDeinit (void)
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

address loomModEndGet (void);
void loomCoreModulesLoad (void);

void export (loomModuleAdd) (loom_module *mod);
bool export (loomModuleRemove) (const char *name);
void export (loomModuleUnload) (loom_module *mod);

#endif