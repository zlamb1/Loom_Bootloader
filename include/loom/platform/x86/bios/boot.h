#ifndef LOOM_BOOT_H
#define LOOM_BOOT_H 1

#define ERROR(x)                                                              \
  mov $x, % si;                                                               \
  call _error

#define PRINT(x)                                                              \
  mov $x, % si;                                                               \
  call _puts

// clang-format off
#define SECTION(name) .section name, "ax", @progbits
// clang-format on

#define PCODE_SEG 0x8
#define PDATA_SEG 0x10
#define RCODE_SEG 0x18
#define RDATA_SEG 0x20

#define SP 0x7C00

#endif