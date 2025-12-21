/**
 * @file platform.h
 * @brief Platform detection and common definitions for PC port
 *
 * This is part of Layer 2 (Porting Abstraction Layer) of the
 * Animal Crossing PC port.
 */
#ifndef PC_PLATFORM_H
#define PC_PLATFORM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Platform detection */
#if defined(PLATFORM_WINDOWS)
    #define PC_WINDOWS 1
#elif defined(PLATFORM_MACOS)
    #define PC_MACOS 1
#elif defined(PLATFORM_LINUX)
    #define PC_LINUX 1
#else
    #error "Unknown platform"
#endif

/* Basic types matching N64/GameCube conventions */
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

/* Boolean type */
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/* Alignment macros */
#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN16(x) ALIGN(x, 16)

/* Array size macro */
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Min/Max macros */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Screen dimensions (can be changed at runtime) */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* N64 original dimensions (for aspect ratio reference) */
#define N64_SCREEN_WIDTH  320
#define N64_SCREEN_HEIGHT 240

/* ============================================================================
 * GameCube Cache Functions - stubs for PC
 * ============================================================================ */
void DCStoreRangeNoSync(void* addr, u32 size);
void DCFlushRange(void* addr, u32 size);
void DCFlushRangeNoSync(void* addr, u32 size);

#endif /* PC_PLATFORM_H */
