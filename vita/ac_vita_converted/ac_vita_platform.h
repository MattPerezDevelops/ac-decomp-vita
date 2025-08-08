/**
 * AC-Decomp Vita Platform Wrapper
 * Drop-in replacement for GameCube SDK - makes AC-Decomp "just work" on Vita
 * 
 * This header automatically:
 * 1. Replaces all GameCube SDK calls with VitaGL equivalents
 * 2. Handles asset loading transparently via asset bridge  
 * 3. Provides identical APIs so AC-Decomp source needs no modification
 */

#pragma once

// Core Vita includes
#include <psp2/types.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/rtc.h>
#include <psp2/audioout.h>

// VitaGL includes  
#include <vitaGL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Prevent GameCube type conflicts with VitaSDK
#define size_t size_t
#define wchar_t wchar_t
#define s32 s32
#define u32 u32

// Basic GameCube types (must be defined first)
typedef float   f32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int8_t   s8;

// Only redefine these if not already defined by VitaSDK
#ifndef s32
typedef int32_t  s32;
#endif
#ifndef u32
typedef uint32_t u32;
#endif

// GameCube/N64 type definitions (including Gfx type)
#include "PR/gbi.h"

// Additional GameCube types that may not be in gbi.h
typedef struct {
    f32 m[4][4];
} Mtx;

typedef struct {
    s16 x, y, z;        // Position
    u16 flag;           // Flags  
    s16 tc[2];          // Texture coordinates
    u8 cn[4];           // Color and normal
} Vtx;

// Asset Bridge
#include "ac_asset_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// AUTOMATIC ASSET LOADING SYSTEM
// =============================================================================

/**
 * Magic macro that automatically replaces AC-Decomp asset includes
 * Usage: Instead of #include "assets/texture.inc", just declare arrays normally
 * and this system will populate them at runtime
 */

// Asset auto-loading macros - these replace the #include system
#define ASSET_TEXTURE_ARRAY(name) \
    static uint8_t* name = NULL; \
    __attribute__((constructor)) \
    static void _load_##name(void) { \
        name = (uint8_t*)ac_get_texture_data(#name); \
    }

#define ASSET_PALETTE_ARRAY(name) \
    static uint16_t* name = NULL; \
    __attribute__((constructor)) \
    static void _load_##name(void) { \
        name = (uint16_t*)ac_get_palette_data(#name); \
    }

#define ASSET_VERTEX_ARRAY(name) \
    static float* name = NULL; \
    __attribute__((constructor)) \
    static void _load_##name(void) { \
        name = (float*)ac_get_vertex_data(#name); \
    }

// For backwards compatibility with AC-Decomp includes
#define ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))

// =============================================================================
// GAMECUBE TO VITA API TRANSLATION
// =============================================================================

// Types translation
typedef uint8_t u8;
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

// Boolean type
typedef int BOOL;
#define TRUE 1
#define FALSE 0

// Memory alignment
#define OSRoundUp32B(x) (((x) + 31) & ~31)
#define OSRoundUp16B(x) (((x) + 15) & ~15)

// =============================================================================
// GRAPHICS SYSTEM (GX -> VitaGL)
// =============================================================================

// GX Graphics types
typedef enum {
    GX_TRIANGLES = GL_TRIANGLES,
    GX_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    GX_TRIANGLE_FAN = GL_TRIANGLE_FAN,
    GX_QUADS = GL_QUADS,
    GX_QUAD_STRIP = GL_TRIANGLE_STRIP,  // VitaGL doesn't have GL_QUAD_STRIP, use triangle strip
    GX_LINES = GL_LINES,
    GX_LINE_STRIP = GL_LINE_STRIP,
    GX_POINTS = GL_POINTS
} GXPrimitive;

typedef enum {
    GX_RGBA8 = GL_RGBA,
    GX_RGB8 = GL_RGB,
    GX_RGB565 = GL_RGB,      // VitaGL doesn't have GL_RGB565, use GL_RGB
    GX_RGBA4 = GL_RGBA,      // VitaGL doesn't have GL_RGBA4, use GL_RGBA
    GX_IA8 = GL_LUMINANCE_ALPHA,
    GX_I8 = GL_LUMINANCE
} GXTexFmt;

// GX Color type
typedef struct {
    u8 r, g, b, a;
} GXColor;

// Graphics initialization
void GXInit(void* base, u32 size);
void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ);
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);

// Drawing functions
void GXBegin(GXPrimitive type, u32 vtxfmt, u16 nverts);
void GXEnd(void);
void GXPosition3f32(f32 x, f32 y, f32 z);
void GXPosition2f32(f32 x, f32 y);
void GXNormal3f32(f32 x, f32 y, f32 z);
void GXColor4u8(u8 r, u8 g, u8 b, u8 a);
void GXTexCoord2f32(f32 s, f32 t);

// Matrix operations
void GXLoadIdentity(void);
void GXLoadPosMtxImm(f32 mtx[3][4], u32 pnmtx);
void GXSetCurrentMtx(u32 mtxid);

// Texture operations
void GXLoadTexObj(void* obj, u32 mapid);
void GXInitTexObj(void* obj, void* img_ptr, u16 width, u16 height, 
                  GXTexFmt format, u32 wrap_s, u32 wrap_t, u32 mipmap);

// Frame buffer
void GXSetCopyClear(GXColor clear_clr, u32 clear_z);
void GXCopyDisp(void* dest, u32 clear);
void GXDrawDone(void);

// =============================================================================
// INPUT SYSTEM (PAD -> sceCtrl)
// =============================================================================

// GameCube controller button mapping
#define PAD_BUTTON_A        0x0001
#define PAD_BUTTON_B        0x0002  
#define PAD_BUTTON_X        0x0004
#define PAD_BUTTON_Y        0x0008
#define PAD_BUTTON_START    0x0010
#define PAD_BUTTON_UP       0x0020
#define PAD_BUTTON_DOWN     0x0040
#define PAD_BUTTON_LEFT     0x0080
#define PAD_BUTTON_RIGHT    0x0100
#define PAD_TRIGGER_L       0x0200
#define PAD_TRIGGER_R       0x0400
#define PAD_TRIGGER_Z       0x0800

// Analog stick values
#define PAD_STICK_RANGE 100

typedef struct {
    u16 button;
    s8 stickX;
    s8 stickY;
    s8 substickX;
    s8 substickY;
    u8 triggerLeft;
    u8 triggerRight;
    u8 analogA;
    u8 analogB;
    u8 err;
} PADStatus;

// Input functions
u32 PADRead(PADStatus* status);
void PADControlMotor(s32 chan, u32 command);

// =============================================================================
// AUDIO SYSTEM 
// =============================================================================

// Audio types and functions
typedef void (*AXAuxCallback)(void* chans, void* context);

void AXInit(void);
void AXQuit(void);
u32 AXGetInputSamples(void);
void AXRegisterAuxACallback(AXAuxCallback callback, void* context);

// =============================================================================
// MEMORY MANAGEMENT
// =============================================================================

// OS Memory functions
void* OSAllocFromHeap(s32 heap, u32 size);
void OSFreeToHeap(s32 heap, void* ptr);
s32 OSCreateHeap(void* start, void* end);

// Cache operations
void DCFlushRange(void* startAddr, u32 nBytes);
void DCInvalidateRange(void* startAddr, u32 nBytes);
void DCStoreRange(void* startAddr, u32 nBytes);
void ICInvalidateRange(void* startAddr, u32 nBytes);

// =============================================================================
// TIME SYSTEM
// =============================================================================

// OS Time
typedef s64 OSTime;
OSTime OSGetTime(void);
u32 OSTicksToMilliseconds(OSTime ticks);
u32 OSTicksToMicroseconds(OSTime ticks);

// =============================================================================
// INITIALIZATION SYSTEM
// =============================================================================

/**
 * Master initialization function - call this once to set up everything
 * This replaces all the individual GC SDK init calls
 */
int AC_Vita_Platform_Init(void);
void AC_Vita_Platform_Cleanup(void);

/**
 * Frame management - call these in your main loop
 */
void AC_Vita_Frame_Begin(void);
void AC_Vita_Frame_End(void);

/**
 * Utility functions for common operations
 */
void AC_Vita_Load_All_Assets_For_Scene(const char* scene_name);
void AC_Vita_Unload_Unused_Assets(void);
void AC_Vita_Print_Performance_Stats(void);

#ifdef __cplusplus
}
#endif 