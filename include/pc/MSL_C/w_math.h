/**
 * @file w_math.h
 * @brief PC override for GameCube MSL_C math wrapper
 *
 * Replaces GameCube-specific intrinsics with standard C math
 */
#ifndef PC_W_MATH_H
#define PC_W_MATH_H

#include <math.h>

/* Use standard C math functions instead of GameCube intrinsics */

static inline float sqrtf_pc(float x) {
    return sqrtf(x);
}

/* Don't redefine sqrtf if using standard library */
#ifndef sqrtf
#define sqrtf sqrtf_pc
#endif

#endif /* PC_W_MATH_H */
