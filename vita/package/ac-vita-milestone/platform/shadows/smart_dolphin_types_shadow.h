/*
 * SMART DOLPHIN TYPES SHADOW
 * ==========================
 * Targets dolphin/types.h (280 errors) - Core type conflicts
 * Resolves fundamental type redefinition issues
 */

#ifndef AC_VITA_SMART_DOLPHIN_TYPES_SHADOW_H
#define AC_VITA_SMART_DOLPHIN_TYPES_SHADOW_H

// ============================================================================
// CONFLICT PREVENTION: Only redefine if conflicts detected
// ============================================================================

#ifdef AC_TYPE_CONFLICTS_DETECTED

// Clear any existing conflicting definitions
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
#ifdef s64
#undef s64
#endif
#ifdef u64
#undef u64
#endif
#ifdef f32
#undef f32
#endif
#ifdef f64
#undef f64
#endif

// ============================================================================
// CLEAN TYPE DEFINITIONS (GameCube standard)
// ============================================================================

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

// ============================================================================
// BOOLEAN AND SYSTEM TYPES
// ============================================================================

#ifndef BOOL
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

#ifndef OSBool
typedef u8 OSBool;
#endif

// ============================================================================
// POINTER AND SIZE TYPES  
// ============================================================================

#ifndef size_t
typedef u32 size_t;
#endif

#ifndef ptrdiff_t
typedef s32 ptrdiff_t;
#endif

// ============================================================================
// MATRIX AND VECTOR TYPES (Graphics system)
// ============================================================================

#ifndef Vec3f
typedef struct {
    f32 x, y, z;
} Vec3f;
#endif

#ifndef Mtx
typedef f32 Mtx[4][4];
#endif

#ifndef MtxF
typedef f32 MtxF[4][4];
#endif

// ============================================================================
// FUNCTION POINTER TYPES
// ============================================================================

#ifndef OSThreadFunc
typedef void* (*OSThreadFunc)(void*);
#endif

#ifndef OSAlarmHandler
typedef void (*OSAlarmHandler)(void*);
#endif

#endif // AC_TYPE_CONFLICTS_DETECTED

#pragma message("Smart Dolphin Types Shadow: 280 type conflicts targeted")

#endif // AC_VITA_SMART_DOLPHIN_TYPES_SHADOW_H 