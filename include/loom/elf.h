#ifndef LOOM_ELF_H
#define LOOM_ELF_H 1

#include "loom/compiler.h"
#include "loom/types.h"

typedef struct
{
#define LOOM_EH_MAG0 0x7F
  u8 magic;
#define LOOM_EH_MAG1 'E'
#define LOOM_EH_MAG2 'L'
#define LOOM_EH_MAG3 'F'
  char sig[3];
#define LOOM_EH_CLASS_32 1
#define LOOM_EH_CLASS_64 2
  u8 class;
#define LOOM_EH_DATA_LE 1
#define LOOM_EH_DATA_BE 2
  u8 data;
  u8 version;
  u8 osabi;
  char pad[8];
#define LOOM_ET_NONE   0
#define LOOM_ET_REL    1
#define LOOM_ET_EXEC   2
#define LOOM_ET_SHARED 3
#define LOOM_ET_CORE   4
  u16 type;
  u16 machine;
  u32 elf_version;
  u32 entry;
  u32 phoff;
  u32 shoff;
  u32 flags;
  u16 size;
  u16 phentsize;
  u16 phents;
  u16 shentsize;
  u16 shents;
  u16 shstridx;
} LOOM_PACKED loom_elf32_ehdr;

typedef struct
{
#define LOOM_PT_NULL    0
#define LOOM_PT_LOAD    1
#define LOOM_PT_DYNAMIC 2
#define LOOM_PT_INTERP  3
#define LOOM_PT_NOTE    4
#define LOOM_PT_SHLIB   5
#define LOOM_PT_PHDR    6
  u32 type;
  u32 offset;
  u32 vaddr;
  u32 paddr;
  u32 filesz;
  u32 memsz;
#define LOOM_PF_X 1
#define LOOM_PF_W 2
#define LOOM_PF_R 4
  u32 flags;
  u32 align;
} LOOM_PACKED LOOM_ALIGNED (1) loom_elf32_phdr;

typedef struct
{
#define LOOM_SHN_UNDEF 0
  u32 name;
#define LOOM_SHT_NULL     0
#define LOOM_SHT_PROGBITS 1
#define LOOM_SHT_SYMTAB   2
#define LOOM_SHT_STRTAB   3
#define LOOM_SHT_RELA     4
#define LOOM_SHT_NOBITS   8
#define LOOM_SHT_REL      9
  u32 type;
#define LOOM_SHF_WRITE     1
#define LOOM_SHF_ALLOC     2
#define LOOM_SHF_EXECINSTR 4
  u32 flags;
  u32 addr;
  u32 offset;
  u32 size;
  u32 link;
  u32 info;
  u32 addralign;
  u32 entsize;
} LOOM_PACKED LOOM_ALIGNED (1) loom_elf32_shdr;

typedef struct
{
  u32 name;
  u32 value;
  u32 size;
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
  u8 info;
  u8 other;
  u16 shidx;
} LOOM_PACKED LOOM_ALIGNED (1) loom_elf32_sym;

typedef struct
{
  u32 offset;
#define LOOM_ELF32_R_SYM(i)  ((i) >> 8)
#define LOOM_ELF32_R_TYPE(i) ((u8) (i))
  u32 info;
} LOOM_PACKED LOOM_ALIGNED (1) loom_elf32_rel;

int LOOM_EXPORT (loom_elf32_ehdr_load) (void *p, usize size,
                                        loom_elf32_ehdr **ehdr);

int LOOM_EXPORT (loom_elf32_shdr_validate) (address addr, usize size,
                                            loom_elf32_shdr *shdr);

int LOOM_EXPORT (loom_elf32_strtab_validate) (loom_elf32_ehdr *ehdr,
                                              usize size,
                                              loom_elf32_shdr *shdr);

loom_elf32_shdr *LOOM_EXPORT (loom_elf32_shdr_get) (loom_elf32_ehdr *ehdr,
                                                    usize shidx);

int LOOM_EXPORT (loom_elf32_shdr_iterate) (
    loom_elf32_ehdr *ehdr, int (*hook) (usize, loom_elf32_shdr *, void *),
    void *data);

int LOOM_EXPORT (loom_elf32_rel_iterate) (
    loom_elf32_ehdr *ehdr,
    int (*hook) (loom_elf32_rel *rel, usize symtabidx, loom_elf32_shdr *symtab,
                 usize targetidx, loom_elf32_shdr *target, void *data),
    void *data);

int loom_elf32_rel_fixup (loom_elf32_rel *rel, loom_elf32_sym *sym, void *b);

#endif