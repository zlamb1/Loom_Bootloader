#ifndef LOOM_MATH_H
#define LOOM_MATH_H 1

#define loom_add __builtin_add_overflow
#define loom_sub __builtin_sub_overflow
#define loom_mul __builtin_mul_overflow

#define loom_checked_cast(x, y) loom_add (x, 0, y)

#endif