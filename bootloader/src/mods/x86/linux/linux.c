#include "loom/command.h"
#include "loom/disk.h"
#include "loom/endian.h"
#include "loom/error.h"
#include "loom/kernel_loader.h"
#include "loom/list.h"
#include "loom/mm.h"
#include "loom/module.h"
#include "loom/string.h"

LOOM_MOD (linux)

extern char stage1s;
extern char stage3e;

typedef struct
{
#define SETUP_HEADER_OFFSET 0x1F1
  loom_uint8_t setup_sects; // if 0, real value is 4
  loom_uint16_t root_flags;
  loom_uint32_t syssize;
  loom_uint16_t ram_size;
  loom_uint16_t vid_mode;
  loom_uint16_t root_dev;
  loom_uint16_t boot_flag; // 0xAA55
  loom_uint16_t jump;
  char header[4]; // HdrS
  loom_uint16_t version;
  loom_uint32_t realmode_swtch;
  loom_uint16_t start_sys_seg;
  loom_uint16_t kernel_version;
  loom_uint8_t type_of_loader;
  loom_uint8_t loadflags;
  loom_uint16_t setup_move_size;
  loom_uint32_t code32_start;
  loom_uint32_t ramdisk_image;
  loom_uint32_t ramdisk_size;
  loom_uint32_t bootsect_kludge;
  loom_uint16_t heap_end_ptr;
  loom_uint8_t ext_loader_ver;
  loom_uint8_t ext_loader_type;
  loom_uint32_t cmd_line_ptr;
  loom_uint32_t initrd_addr_max;
  loom_uint32_t kernel_alignment;
  loom_uint8_t relocatable_kernel;
  loom_uint8_t min_alignment;
  loom_uint16_t xloadflags;
  loom_uint32_t cmdline_size;
  loom_uint32_t hardware_subarch;
  loom_uint64_t hardware_subarch_data;
  loom_uint32_t payload_offset;
  loom_uint32_t payload_length;
  loom_uint64_t setup_data;
  loom_uint64_t pref_address;
  loom_uint32_t init_size;
  loom_uint32_t handover_offset;
  loom_uint32_t kernel_info_offset;
} LOOM_PACKED setup_header_t;

static loom_kernel_loader_t linux_loader = { 0 };

static void
linux_boot (loom_kernel_loader_t *kernel_loader)
{
  extern char linux_relocator;
  extern char linux_relocator_end;

  (void) linux_loader;

#define SCRATCH 0x60000
#define SEG     0x8000

  char *kernel = kernel_loader->kernel;
  loom_usize_t off, setup_sects;

  setup_header_t *setup_header
      = (setup_header_t *) ((char *) kernel_loader->kernel
                            + SETUP_HEADER_OFFSET);

  setup_sects = setup_header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  off = (setup_sects + 1) * 512;
  loom_memcpy ((void *) (SEG * 0x10), kernel, off);

  loom_memcpy ((void *) SCRATCH, &linux_relocator,
               (loom_usize_t) (&linux_relocator_end - &linux_relocator));

  ((void (*) (loom_uint32_t dst, loom_uint32_t src, loom_uint32_t size,
              loom_uint16_t seg)) SCRATCH) (
      setup_header->code32_start, (loom_uint32_t) kernel_loader->kernel + off,
      kernel_loader->kernel_size - off, SEG);
}

static int
linux_task (LOOM_UNUSED loom_command_t *cmd, LOOM_UNUSED loom_usize_t argc,
            LOOM_UNUSED char *argv[])
{
  loom_module_header_t hdr;
  loom_disk_t *disk;
  loom_usize_t offset, kernel_size;
  loom_uint32_t setup_sects;

  setup_header_t *setup_header;
  char *kbuf = NULL, *cmdline = NULL;

  loom_kernel_loader_remove (1);

  loom_memcpy (&hdr, (void *) loom_modbase, sizeof (hdr));

  offset = (loom_usize_t) &stage3e - (loom_usize_t) &stage1s;
  offset += hdr.size;

  kernel_size = loom_le32toh (hdr.kernel_size);

  if (!kernel_size)
    {
      loom_error (LOOM_ERR_BAD_ARG, "no kernel appended");
      return -1;
    }

  kbuf = loom_malloc (kernel_size);

  if (!kbuf)
    return -1;

  if (loom_list_is_empty (&loom_disks))
    {
      loom_error (LOOM_ERR_IO, "no disks found");
      goto out;
    }

  disk = loom_container_of (loom_disks.next, loom_disk_t, node);

  if (loom_disk_read (disk, offset, kernel_size, kbuf))
    goto out;

  setup_header = (setup_header_t *) (kbuf + SETUP_HEADER_OFFSET);

  if (loom_memcmp (setup_header->header, "HdrS", 4))
    {
      loom_error (LOOM_ERR_BAD_ARG, "invalid kernel header");
      goto out;
    }

  setup_sects = setup_header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  offset = (setup_sects + 1) * 512;

  setup_header->vid_mode = 0xFFFF;
  setup_header->type_of_loader = 0xFF;
  setup_header->loadflags |= 0x80;
  setup_header->ramdisk_image = 0;
  setup_header->ramdisk_size = 0;
  setup_header->heap_end_ptr = 0;
  setup_header->setup_data = 0;

  linux_loader.boot = linux_boot;
  linux_loader.flags = 0;
  linux_loader.kernel_size = kernel_size;
  linux_loader.kernel = kbuf;

  loom_kernel_loader_add (&linux_loader);

  return 0;

out:
  loom_free (kbuf);
  loom_free (cmdline);
  return -1;
}

int linux_task (loom_command_t *cmd, loom_usize_t argc, char *argv[]);

static loom_command_t linux_command = {
  .name = "linux",
  .task = linux_task,
};

LOOM_MOD_INIT () { loom_command_register (&linux_command); }

LOOM_MOD_DEINIT () { loom_command_unregister (&linux_command); }