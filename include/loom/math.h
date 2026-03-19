#ifndef LOOM_MATH_H
#define LOOM_MATH_H 1

#define loomAdd __builtin_add_overflow
#define loomSub __builtin_sub_overflow
#define loomMul __builtin_mul_overflow

#define loomCheckedCast(x, y) loomAdd (x, 0, y)

#endif