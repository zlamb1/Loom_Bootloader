#ifndef _LOOM_KEYCODE_H
#define _LOOM_KEYCODE_H 1

#include "compiler.h"

#define LOOM_KEY_A          1
#define LOOM_KEY_B          2
#define LOOM_KEY_C          3
#define LOOM_KEY_D          4
#define LOOM_KEY_E          5
#define LOOM_KEY_F          6
#define LOOM_KEY_G          7
#define LOOM_KEY_H          8
#define LOOM_KEY_I          9
#define LOOM_KEY_J          10
#define LOOM_KEY_K          11
#define LOOM_KEY_L          12
#define LOOM_KEY_M          13
#define LOOM_KEY_N          14
#define LOOM_KEY_O          15
#define LOOM_KEY_P          16
#define LOOM_KEY_Q          17
#define LOOM_KEY_R          18
#define LOOM_KEY_S          19
#define LOOM_KEY_T          20
#define LOOM_KEY_U          21
#define LOOM_KEY_V          22
#define LOOM_KEY_W          23
#define LOOM_KEY_X          24
#define LOOM_KEY_Y          25
#define LOOM_KEY_Z          26
#define LOOM_KEY_0          27
#define LOOM_KEY_1          28
#define LOOM_KEY_2          29
#define LOOM_KEY_3          30
#define LOOM_KEY_4          31
#define LOOM_KEY_5          32
#define LOOM_KEY_6          33
#define LOOM_KEY_7          34
#define LOOM_KEY_8          35
#define LOOM_KEY_9          36
#define LOOM_KEY_TILDE      37
#define LOOM_KEY_MINUS      38
#define LOOM_KEY_EQUAL      39
#define LOOM_KEY_LEFTBRACE  40
#define LOOM_KEY_RIGHTBRACE 41
#define LOOM_KEY_BACKSLASH  42
#define LOOM_KEY_SEMICOLON  43
#define LOOM_KEY_APOSTROPHE 44
#define LOOM_KEY_COMMA      45
#define LOOM_KEY_PERIOD     46
#define LOOM_KEY_SLASH      47
#define LOOM_KEY_ESCAPE     48
#define LOOM_KEY_TAB        49
#define LOOM_KEY_BACKSPACE  50
#define LOOM_KEY_CAPSLOCK   51
#define LOOM_KEY_ENTER      52
#define LOOM_KEY_LEFTSHIFT  53
#define LOOM_KEY_RIGHTSHIFT 54
#define LOOM_KEY_LEFTCTRL   55
#define LOOM_KEY_SUPER      56
#define LOOM_KEY_LEFTALT    57
#define LOOM_KEY_SPACE      58
#define LOOM_KEY_RIGHTALT   59
#define LOOM_KEY_FN         60
#define LOOM_KEY_RIGHTCTRL  61
#define LOOM_KEY_LEFT       62
#define LOOM_KEY_RIGHT      63
#define LOOM_KEY_UP         64
#define LOOM_KEY_DOWN       65
#define LOOM_KEY_F1         66
#define LOOM_KEY_F2         67
#define LOOM_KEY_F3         68
#define LOOM_KEY_F4         69
#define LOOM_KEY_F5         70
#define LOOM_KEY_F6         71
#define LOOM_KEY_F7         72
#define LOOM_KEY_F8         73
#define LOOM_KEY_F9         74
#define LOOM_KEY_F10        75
#define LOOM_KEY_F11        76
#define LOOM_KEY_F12        77
#define LOOM_KEY_F13        78
#define LOOM_KEY_F14        79
#define LOOM_KEY_F15        80
#define LOOM_KEY_F16        81
#define LOOM_KEY_F17        82
#define LOOM_KEY_F18        83
#define LOOM_KEY_F19        84
#define LOOM_KEY_F20        85
#define LOOM_KEY_F21        86
#define LOOM_KEY_F22        87
#define LOOM_KEY_F23        88
#define LOOM_KEY_F24        89
#define LOOM_KEY_PRTSC      90
#define LOOM_KEY_PAUSE      91
#define LOOM_KEY_SCROLLLOCK 92
#define LOOM_KEY_NUMLOCK    93
#define LOOM_KEY_INSERT     94
#define LOOM_KEY_HOME       95
#define LOOM_KEY_DELETE     96
#define LOOM_KEY_END        97
#define LOOM_KEY_PAGEUP     98
#define LOOM_KEY_PAGEDOWN   99
#define LOOM_KEY_NP0        100
#define LOOM_KEY_NP1        101
#define LOOM_KEY_NP2        102
#define LOOM_KEY_NP3        103
#define LOOM_KEY_NP4        104
#define LOOM_KEY_NP5        105
#define LOOM_KEY_NP6        106
#define LOOM_KEY_NP7        107
#define LOOM_KEY_NP8        108
#define LOOM_KEY_NP9        109
#define LOOM_KEY_NPSLASH    110
#define LOOM_KEY_NPASTERISK 111
#define LOOM_KEY_NPMINUS    112
#define LOOM_KEY_NPPLUS     113
#define LOOM_KEY_NPPERIOD   114
#define LOOM_KEY_NPENTER    115

static UNUSED int
loom_keycode_alpha (int keycode)
{
  return keycode >= LOOM_KEY_A && keycode <= LOOM_KEY_Z;
}

static UNUSED int
loom_keycode_numeric (int keycode)
{
  return keycode >= LOOM_KEY_0 && keycode <= LOOM_KEY_9;
}

char EXPORT (loom_keycode_to_char) (int mods, int keycode);

#endif
