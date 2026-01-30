#include "loom/command.h"
#include "loom/disk.h"
#include "loom/error.h"
#include "loom/kernel_loader.h"
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
} PACKED setup_header_t;

static loom_kernel_loader_t linux_loader = {};

static int
linux_task (UNUSED loom_command_t *cmd, UNUSED loom_usize_t argc,
            UNUSED char *argv[])
{
  loom_module_header_t hdr;
  loom_disk_t *disk;
  loom_usize_t offset, kernel_size;
  loom_uint32_t setup_sects;

  setup_header_t *setup_header;
  char *kbuf = NULL, *cmdline = NULL;

  loom_memcpy (&hdr, (void *) loom_modbase, sizeof (hdr));

  offset = (loom_usize_t) &stage3e - (loom_usize_t) &stage1s;
  offset += hdr.size;

  kernel_size = hdr.kernel_size;

  if (!kernel_size)
    {
      loom_error (LOOM_ERR_BAD_ARG, "no kernel appended");
      return -1;
    }

  kbuf = loom_malloc (kernel_size);

  if (!kbuf)
    return -1;

  disk = loom_disks;

  if (!disk)
    {
      loom_error (LOOM_ERR_IO, "no disk found");
      goto out;
    }

  if (loom_disk_read (disk, offset, kernel_size, kbuf))
    goto out;

  setup_header = (setup_header_t *) (kbuf + SETUP_HEADER_OFFSET);

  if (loom_memcmp (setup_header->header, "HdrS", 4))
    {
      loom_error (LOOM_ERR_BAD_ARG, "invalid kernel header");
      goto out;
    }

  cmdline = loom_malloc (5);

  if (!cmdline)
    goto out;

  loom_memcpy (cmdline, "auto", 5);

  setup_sects = setup_header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  setup_header->vid_mode = 0xFFFF;
  setup_header->type_of_loader = 0xFF;
  setup_header->loadflags |= 0x80;
  setup_header->ramdisk_image = 0;
  setup_header->ramdisk_size = 0;
  setup_header->heap_end_ptr = 0;
  setup_header->cmd_line_ptr = (loom_uint32_t) cmdline;
  setup_header->setup_data = 0;

  /*loom_memcpy ((void *) (seg * 0x10), kbuf, (setup_sects + 1) * 512);

  loom_memcpy ((void *) (0x90000), "auto quiet console=tty0", 24);

  loom_memmove ((void *) 0x100000, kbuf + kernel32_offset,
                kernel_size - kernel32_offset);

  loom_boot_linux (seg);
  */

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