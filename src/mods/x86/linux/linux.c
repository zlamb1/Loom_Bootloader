#include "loom/command.h"
#include "loom/error.h"
#include "loom/file.h"
#include "loom/fs.h"
#include "loom/kernel_loader.h"
#include "loom/math.h"
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
linuxBoot (loom_kernel_loader *loader)
{
  extern char linuxRelocator;
  extern char linuxRelocatorEnd;

#define SCRATCH 0x500
#define SEG     0x8000

  auto kernel = loader->kernel;
  auto initrd = loader->initrd;
  auto cmdline = loader->cmdline;

  usize off, setup_sects;

  setup_header *header
      = (setup_header *) ((char *) kernel.data + SETUP_HEADER_OFFSET);

  setup_sects = header->setup_sects;
  if (!setup_sects)
    setup_sects = 4;

  if (initrd.data != null)
    {
      header->ramdisk_image = (u32) initrd.data;
      header->ramdisk_size = (u32) initrd.size;
    }

  if (cmdline.data != null)
    {
      header->cmdline_size = cmdline.size;
      header->cmd_line_ptr = (u32) cmdline.data;
    }

  off = (setup_sects + 1) * 512;
  loomMemCopy ((void *) (SEG * 0x10), kernel.data, off);

  loomMemCopy ((void *) SCRATCH, &linuxRelocator,
               (usize) (&linuxRelocatorEnd - &linuxRelocator));

  ((void (*) (u32 dst, u32 src, u32 size, u16 seg)) SCRATCH) (
      header->code32_start, (u32) kernel.data + off, kernel.size - off, SEG);
}

static int
linuxTask (unused loom_command *cmd, unused usize argc, unused char *argv[])
{
  loom_file kfile;

  u32 setup_sects;

  setup_header *header;
  char *kbuf = NULL;

  loomKernelLoaderRemove ();

  if (loom_prefix_fs == null)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "set prefix to a valid fs");
      return -1;
    }

  if (argc < 2)
    {
      loomErrorFmt (LOOM_ERR_BAD_ARG, "provide a file path");
      return -1;
    }

  if (loomFileOpen (loom_prefix_fs, &kfile, argv[1]))
    return -1;

  kbuf = loomAlloc (kfile.size);
  if (kbuf == null)
    goto out;

  if (loomFileRead (&kfile, kfile.size, kbuf, null))
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

  header->vid_mode = 0xFFFF;
  header->type_of_loader = 0xFF;
  header->loadflags |= 0x80;
  header->ramdisk_size = 0;
  header->ramdisk_image = 0;
  header->cmdline_size = 0;
  header->cmd_line_ptr = 0;
  header->heap_end_ptr = 0;
  header->setup_data = 0;

  linux_loader.boot = linuxBoot;
  linux_loader.flags = LOOM_KERNEL_LOADER_FLAG_INITRD;

  if (header->pref_address < U32_MAX)
    {
      auto pref_address = (u32) header->pref_address;
      if (loomAdd (pref_address, header->init_size,
                   &linux_loader.initrd_min_addr))
        linux_loader.initrd_min_addr = 0;
    }
  else
    linux_loader.initrd_min_addr = 0x100000 + kfile.size;

  linux_loader.initrd_max_addr = header->initrd_addr_max;

  linux_loader.initrd_max_addr = header->initrd_addr_max;
  linux_loader.kernel.size = kfile.size;
  linux_loader.kernel.data = kbuf;

  if (argc > 2)
    {
      // FIXME: This leaks if loader is removed.
      auto cmdline = &linux_loader.cmdline;

      for (usize i = 2; i < argc; ++i)
        {
          auto length = loomStrLength (argv[i]);
          if (length == 0)
            continue;

          if (length == USIZE_MAX
              || loomAdd (cmdline->size, length + 1, &cmdline->size))
            {
              cmdline->size = 0;
              loomErrorFmt (LOOM_ERR_OVERFLOW,
                            "too many command line arguments");
              goto out;
            }
        }

      if (cmdline->size > 0)
        {
          usize offset = 0;

          cmdline->data
              = loomMemAlignRangeHigh (cmdline->size, 0x8, 0, USIZE_MAX);
          auto data = (char *) cmdline->data;

          if (data == null)
            goto out;

          for (usize i = 2; i < argc; ++i)
            {
              auto length = loomStrLength (argv[i]);
              if (length == 0)
                continue;
              loomMemCopy (data + offset, argv[i], length);
              offset += length;
              data[offset++] = (i == argc - 1 ? '\0' : ' ');
            }

          // Final size does not include NUL terminator.
          cmdline->size -= 1;
        }
    }
  else
    {
      header->cmd_line_ptr = 0;
      header->cmdline_size = 0;
    }

  loomKernelLoaderSet (&linux_loader);

  return 0;

out:
  loomFileClose (&kfile);
  loomFree (kbuf);
  return -1;
}

static loom_command linux_command = {
  .name = "linux",
  .task = linuxTask,
};

LOOM_MOD_INIT () { loomCommandRegister (&linux_command); }

LOOM_MOD_DEINIT () { loomCommandUnregister (&linux_command); }