/**
 * VitaGL → GameCube API Wrapper
 * 
 * This provides GameCube graphics API functions that translate to VitaGL calls.
 * AC-decomp source calls GameCube functions → our wrapper translates to VitaGL.
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

// VitaGL (OpenGL 1.x Fixed Pipeline) - our target graphics API
#include <vitaGL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Prevent GameCube type conflicts with VitaSDK
#define size_t size_t
#define wchar_t wchar_t

// Basic GameCube types for AC-decomp compatibility
typedef float   f32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int8_t   s8;
typedef int32_t  s32;
typedef uint32_t u32;

// GameCube/N64 type definitions (including Gfx type)
#include "PR/gbi.h"

// Additional GameCube graphics types
typedef struct {
    f32 m[4][4];
} Mtx;

typedef struct {
    s16 x, y, z;        // Position
    u16 flag;           // Flags  
    s16 tc[2];          // Texture coordinates
    u8 cn[4];           // Color and normal
} Vtx;

// GameCube graphics enums that translate to VitaGL/OpenGL constants
typedef enum {
    GX_TRIANGLES = GL_TRIANGLES,
    GX_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    GX_TRIANGLE_FAN = GL_TRIANGLE_FAN,
    GX_QUADS = GL_QUADS,
    GX_QUAD_STRIP = GL_TRIANGLE_STRIP,  // VitaGL doesn't have GL_QUAD_STRIP
    GX_LINES = GL_LINES,
    GX_LINE_STRIP = GL_LINE_STRIP,
    GX_POINTS = GL_POINTS
} GXPrimitive;

typedef enum {
    GX_RGBA8 = GL_RGBA,
    GX_RGB8 = GL_RGB,
    GX_RGB565 = GL_RGB,      // VitaGL approximation
    GX_RGBA4 = GL_RGBA,      // VitaGL approximation
    GX_IA8 = GL_LUMINANCE_ALPHA,
    GX_I8 = GL_LUMINANCE
} GXTexFmt;

typedef struct {
    u8 r, g, b, a;
} GXColor;

// GameCube API Functions → VitaGL Implementation
// These are the wrapper functions that AC-decomp will call

// Graphics Initialization
void GXInit(void* base, u32 size);

// Scissor and Viewport
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);

// Primitive Rendering (GameCube API → VitaGL calls)
void GXBegin(GXPrimitive type, u32 vtxfmt, u16 nverts);
void GXEnd(void);
void GXPosition3f32(f32 x, f32 y, f32 z);
void GXPosition2f32(f32 x, f32 y);
void GXNormal3f32(f32 x, f32 y, f32 z);
void GXColor4u8(u8 r, u8 g, u8 b, u8 a);
void GXTexCoord2f32(f32 s, f32 t);

// Matrix Operations (GameCube API → VitaGL matrix calls)
void GXLoadPosMtxImm(f32 mtx[3][4], u32 pnmtx);
void GXSetCurrentMtx(u32 mtxid);

// Texture Operations (GameCube API → VitaGL texture calls)
void GXLoadTexObj(void* obj, u32 mapid);
void GXInitTexObj(void* obj, void* img_ptr, u16 width, u16 height,
                  u32 format, u32 wrap_s, u32 wrap_t, u32 mipmap);

// Framebuffer Operations
void GXSetCopyClear(GXColor clear_clr, u32 clear_z);
void GXCopyDisp(void* dest, u32 clear);

// Asset Bridge System
#include "ac_asset_bridge.h"

// Memory and utility macros (GameCube → Vita)
#define OSRoundUp32B(x) (((x) + 31) & ~31)
#define ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))

// Cache operations (GameCube API → Vita no-ops or equivalents)
void DCFlushRange(void* startAddr, u32 nBytes);
void DCInvalidateRange(void* startAddr, u32 nBytes);
void DCStoreRange(void* startAddr, u32 nBytes);
void ICInvalidateRange(void* startAddr, u32 nBytes); 