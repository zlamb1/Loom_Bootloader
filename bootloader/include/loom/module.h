#ifndef LOOM_MODULE_H
#define LOOM_MODULE_H 1

#include "crypto/crypto.h"
#include "loom/compiler.h"
#include "loom/crypto/sha1.h"
#include "loom/types.h"

#ifdef LOOM_MODULE
#define LOOM_MOD(NAME)    const char USED *loom_mod_name = #NAME;
#define LOOM_MOD_INIT()   void USED loom_mod_init (void)
#define LOOM_MOD_DEINIT() void USED loom_mod_deinit (void)
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
} PACKED loom_module_header_t;

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
#ifdef LOOM_DEBUG
  loom_digest_t hash[LOOM_SHA1_DIGEST_SIZE];
#endif
  loom_module_init_t init;
  loom_module_deinit_t deinit;
  loom_module_section_t *sections;
  struct loom_module_t *prev, *next;
} loom_module_t;

extern loom_address_t EXPORT_VAR (loom_modbase);
extern loom_address_t EXPORT_VAR (loom_modend);

extern loom_module_t *EXPORT_VAR (loom_modules);

loom_address_t loom_modend_get (void);
void loom_core_modules_load (void);

void EXPORT (loom_module_add) (loom_module_t *mod);
loom_bool_t EXPORT (loom_module_remove) (const char *name);
void EXPORT (loom_module_unload) (loom_module_t *mod);

#endif