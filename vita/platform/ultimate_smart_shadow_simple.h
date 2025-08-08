/*
 * ULTIMATE SMART SHADOW - SIMPLE VERSION
 * ======================================
 * Simplified for forced inclusion compatibility
 * No runtime functions, pure preprocessor logic
 */

#ifndef AC_VITA_ULTIMATE_SMART_SHADOW_SIMPLE_H
#define AC_VITA_ULTIMATE_SMART_SHADOW_SIMPLE_H

// ============================================================================
// CORE TYPE FIXES - Always apply these safely
// ============================================================================

// Prevent multiple redefinitions
#ifndef AC_VITA_TYPES_FIXED
#define AC_VITA_TYPES_FIXED 1

// Clear any existing problematic definitions
#ifdef s8
#undef s8
#endif
#ifdef u8
#undef u8
#endif
#ifdef s16
#undef s16
#endif
#ifdef u16
#undef u16
#endif
#ifdef s32
#undef s32
#endif
#ifdef u32
#undef u32
#endif
#ifdef f32
#undef f32
#endif

// Clean type definitions
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;
typedef double f64;

#endif // AC_VITA_TYPES_FIXED

// ============================================================================
// GRAPHICS FIXES - Essential for AC-decomp
// ============================================================================

#ifndef AC_VITA_GFX_FIXED
#define AC_VITA_GFX_FIXED 1

// Gfx union fix
#ifdef Gfx
#undef Gfx
#endif

typedef union {
    unsigned long long force_structure_alignment;
    struct {
        unsigned int w0;
        unsigned int w1;
    } words;
} Gfx;

#endif // AC_VITA_GFX_FIXED

// ============================================================================
// ESSENTIAL CONSTANTS - Safe definitions
// ============================================================================

#ifndef AC_VITA_CONSTANTS_FIXED
#define AC_VITA_CONSTANTS_FIXED 1

// Field constants
#ifndef mFI_UNIT_BASE_SIZE_F
#define mFI_UNIT_BASE_SIZE_F 160.0f
#endif

#ifndef mFI_UT_WORLDSIZE_X_F
#define mFI_UT_WORLDSIZE_X_F 1280.0f
#endif

#ifndef mFI_UT_WORLDSIZE_Z_F
#define mFI_UT_WORLDSIZE_Z_F 1280.0f
#endif

// Alignment macros
#ifndef ALIGN_NEXT
#define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
#endif

#ifndef mCD_ALIGN_SECTORSIZE
#define mCD_ALIGN_SECTORSIZE(x) 0x30000
#endif

// Utility macros
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef GRAPH_ALLOC_TYPE
#define GRAPH_ALLOC_TYPE(graph, type, count) ((type*)malloc(sizeof(type) * (count)))
#endif

#endif // AC_VITA_CONSTANTS_FIXED

// ============================================================================
// BOOLEAN TYPES - Safe definitions
// ============================================================================

#ifndef BOOL
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

// Success marker - no runtime calls
#ifdef __GNUC__
#pragma message("Ultimate Smart Shadow Simple: Safe forced inclusion applied")
#endif

#endif // AC_VITA_ULTIMATE_SMART_SHADOW_SIMPLE_H 