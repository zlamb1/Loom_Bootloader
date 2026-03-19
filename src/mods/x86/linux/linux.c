#include "loom/block_dev.h"
#include "loom/command.h"
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

typedef struct setup_header
{
#define SETUP_HEADER_OFFSET 0x1F1
  u8 setup_sects; // if 0, real value is 4
  u16 root_flags;
  u32 syssize;
  u16 ram_size;
  u16 vid_mode;
  u16 root_dev;
  u16 boot_flag; // 0xAA55
  u16 jump;
  char header[4]; // HdrS
  u16 version;
  u32 realmode_swtch;
  u16 start_sys_seg;
  u16 kernel_version;
  u8 type_of_loader;
  u8 loadflags;
  u16 setup_move_size;
  u32 code32_start;
  u32 ramdisk_image;
  u32 ramdisk_size;
  u32 bootsect_kludge;
  u16 heap_end_ptr;
  u8 ext_loader_ver;
  u8 ext_loader_type;
  u32 cmd_line_ptr;
  u32 initrd_addr_max;
  u32 kernel_alignment;
  u8 relocatable_kernel;
  u8 min_alignment;
  u16 xloadflags;
  u32 cmdline_size;
  u32 hardware_subarch;
  u64 hardware_subarch_data;
  u32 payload_offset;
  u32 payload_length;
  u64 setup_data;
  u64 pref_address;
  u32 init_size;
  u32 handover_offset;
  u32 kernel_info_offset;
} packed setup_header;

static loom_kernel_loader linux_loader = { 0 };

static void
linuxBoot (loom_kernel_loader *kernel_loader)
{
  extern char linux_relocator;
  extern char linux_relocator_end;

  (void) linux_loader;

#define SCRATCH 0x60000
#define SEG     0x8000

  char *kernel = kernel_loader->kernel;
  usize off, setup_sects;

  setup_header *header = (setup_header *) ((char *) kernel_loader->kernel
                                           + SETUP_HEADER_OFFSET);

  setup_sects = header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  off = (setup_sects + 1) * 512;
  loomMemCopy ((void *) (SEG * 0x10), kernel, off);

  loomMemCopy ((void *) SCRATCH, &linux_relocator,
               (usize) (&linux_relocator_end - &linux_relocator));

  ((void (*) (u32 dst, u32 src, u32 size, u16 seg)) SCRATCH) (
      header->code32_start, (u32) kernel_loader->kernel + off,
      kernel_loader->kernel_size - off, SEG);
}

static int
linuxTask (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_module_header hdr;
  loom_block_dev *block_dev;
  usize offset, kernel_size;
  u32 setup_sects;

  setup_header *header;
  char *kbuf = NULL, *cmdline = NULL;

  loomKernelLoaderRemove (true);

  loomMemCopy (&hdr, (void *) loom_modbase, sizeof (hdr));

  offset = (usize) &stage3e - (usize) &stage1s;
  offset += hdr.size;

  kernel_size = loom_le32toh (hdr.kernel_size);

  if (!kernel_size)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "no kernel appended");
      return -1;
    }

  kbuf = loomAlloc (kernel_size);

  if (!kbuf)
    return -1;

  if (loomListIsEmpty (&loom_block_devs))
    {
      loomErrorFmt (LOOM_ERR_IO, "no disks found");
      goto out;
    }

  block_dev = container_of (loom_block_devs.next, loom_block_dev, node);

  if (loomBlockDevRead (block_dev, offset, kernel_size, kbuf))
    goto out;

  header = (setup_header *) (kbuf + SETUP_HEADER_OFFSET);

  if (loomMemCmp (header->header, "HdrS", 4))
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "invalid kernel header");
      goto out;
    }

  setup_sects = header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  offset = (setup_sects + 1) * 512;

  header->vid_mode = 0xFFFF;
  header->type_of_loader = 0xFF;
  header->loadflags |= 0x80;
  header->ramdisk_image = 0;
  header->ramdisk_size = 0;
  header->heap_end_ptr = 0;
  header->setup_data = 0;

  linux_loader.boot = linuxBoot;
  linux_loader.flags = 0;
  linux_loader.kernel_size = kernel_size;
  linux_loader.kernel = kbuf;

  loomKernelLoaderSet (&linux_loader);

  return 0;

out:
  loomFree (kbuf);
  loomFree (cmdline);
  return -1;
}

static loom_command linux_command = {
  .name = "linux",
  .task = linuxTask,
};

LOOM_MOD_INIT () { loomCommandRegister (&linux_command); }

LOOM_MOD_DEINIT () { loomCommandUnregister (&linux_command); }