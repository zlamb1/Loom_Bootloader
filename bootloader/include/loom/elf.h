#ifndef LOOM_ELF_H
#define LOOM_ELF_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
#define LOOM_EH_MAGIC 0x7F
  loom_uint8_t magic;
#define LOOM_EH_SIG "ELF"
  char sig[3];
#define LOOM_EH_CLASS_32 1
#define LOOM_EH_CLASS_64 2
  loom_uint8_t class;
#define LOOM_EH_DATA_LE 1
#define LOOM_EH_DATA_BE 2
  loom_uint8_t data;
  loom_uint8_t version;
  loom_uint8_t osabi;
  char pad[8];
#define LOOM_ET_NONE   0
#define LOOM_ET_REL    1
#define LOOM_ET_EXEC   2
#define LOOM_ET_SHARED 3
#define LOOM_ET_CORE   4
  loom_uint16_t type;
  loom_uint16_t machine;
  loom_uint32_t elf_version;
  loom_uint32_t entry;
  loom_uint32_t phoff;
  loom_uint32_t shoff;
  loom_uint32_t flags;
  loom_uint16_t size;
  loom_uint16_t phentsize;
  loom_uint16_t phents;
  loom_uint16_t shentsize;
  loom_uint16_t shents;
  loom_uint16_t shstridx;
} PACKED loom_elf32_ehdr_t;

typedef struct
{
#define LOOM_PT_NULL    0
#define LOOM_PT_LOAD    1
#define LOOM_PT_DYNAMIC 2
#define LOOM_PT_INTERP  3
#define LOOM_PT_NOTE    4
#define LOOM_PT_SHLIB   5
#define LOOM_PT_PHDR    6
  loom_uint32_t type;
  loom_uint32_t offset;
  loom_uint32_t vaddr;
  loom_uint32_t paddr;
  loom_uint32_t filesz;
  loom_uint32_t memsz;
#define LOOM_PF_X 1
#define LOOM_PF_W 2
#define LOOM_PF_R 4
  loom_uint32_t flags;
  loom_uint32_t align;
} PACKED ALIGNED (1) loom_elf32_phdr_t;

typedef struct
{
#define LOOM_SHN_UNDEF 0
  loom_uint32_t name;
#define LOOM_SHT_NULL     0
#define LOOM_SHT_PROGBITS 1
#define LOOM_SHT_SYMTAB   2
#define LOOM_SHT_STRTAB   3
#define LOOM_SHT_RELA     4
#define LOOM_SHT_NOBITS   8
#define LOOM_SHT_REL      9
  loom_uint32_t type;
#define LOOM_SHF_WRITE     1
#define LOOM_SHF_ALLOC     2
#define LOOM_SHF_EXECINSTR 4
  loom_uint32_t flags;
  loom_uint32_t addr;
  loom_uint32_t offset;
  loom_uint32_t size;
  loom_uint32_t link;
  loom_uint32_t info;
  loom_uint32_t addralign;
  loom_uint32_t entsize;
} PACKED ALIGNED (1) loom_elf32_shdr_t;

typedef struct
{
  loom_uint32_t name;
  loom_uint32_t value;
  loom_uint32_t size;
#define LOOM_STB_LOCAL        0
#define LOOM_STB_GLOBAL       1
#define LOOM_STB_WEAK         2
#define LOOM_STT_NOTYPE       0
#define LOOM_STT_OBJECT       1
#define LOOM_STT_FUNC         2
#define LOOM_STT_SECTION      3
#define LOOM_STT_FILE         4
#define LOOM_ELF32_ST_BIND(i) ((i) >> 4)
#define LOOM_ELF32_ST_TYPE(i) ((i) & 0xF)
  loom_uint8_t info;
  loom_uint8_t other;
  loom_uint16_t shidx;
} PACKED ALIGNED (1) loom_elf32_sym_t;

typedef struct
{
  loom_uint32_t offset;
#define LOOM_ELF32_R_SYM(i)  ((i) >> 8)
#define LOOM_ELF32_R_TYPE(i) ((loom_uint8_t) (i))
  loom_uint32_t info;
} PACKED ALIGNED (1) loom_elf32_rel_t;

int EXPORT (loom_elf32_ehdr_load) (void *p, loom_usize_t size,
                                   loom_elf32_ehdr_t **ehdr);

int EXPORT (loom_elf32_shdr_validate) (loom_address_t addr, loom_usize_t size,
                                       loom_elf32_shdr_t *shdr);

int EXPORT (loom_elf32_strtab_validate) (loom_elf32_ehdr_t *ehdr,
                                         loom_usize_t size,
                                         loom_elf32_shdr_t *shdr);

loom_elf32_shdr_t *EXPORT (loom_elf32_shdr_get) (loom_elf32_ehdr_t *ehdr,
                                                 loom_usize_t shidx);

int EXPORT (loom_elf32_shdr_iterate) (
    loom_elf32_ehdr_t *ehdr,
    int (*hook) (loom_usize_t, loom_elf32_shdr_t *, void *), void *data);

int EXPORT (loom_elf32_rel_iterate) (
    loom_elf32_ehdr_t *ehdr,
    int (*hook) (loom_elf32_rel_t *rel, loom_usize_t symtabidx,
                 loom_elf32_shdr_t *symtab, loom_usize_t targetidx,
                 loom_elf32_shdr_t *target, void *data),
    void *data);

int loom_elf32_rel_fixup (loom_elf32_rel_t *rel, loom_elf32_sym_t *sym,
                          void *b);

#endif