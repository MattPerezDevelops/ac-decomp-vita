#ifndef GX_TO_VITAGL_WRAPPER_H
#define GX_TO_VITAGL_WRAPPER_H

/**
 * Comprehensive GameCube GX → VitaGL Graphics Wrapper
 * 
 * This header provides a complete translation layer between GameCube's GX graphics API
 * and PlayStation Vita's VitaGL (OpenGL ES 1.1) implementation.
 * 
 * Based on analysis of AC-Decomp usage patterns:
 * - GXBegin/GXEnd primitive rendering
 * - GXPosition/GXTexCoord vertex specification  
 * - GXInitTexObj texture management
 * - Matrix transformations and camera setup
 * 
 * Status: Foundation for Animal Crossing Vita port
 */

#include <vitaGL.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// GameCube/N64 type definitions (including Gfx type)
#include "PR/gbi.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GX Type Definitions (Essential GameCube Types)
// ============================================================================

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef float f32;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// GX Color structure
typedef struct {
    u8 r, g, b, a;
} GXColor;

// GX Texture Object (simplified for VitaGL)
typedef struct {
    GLuint texture_id;
    u16 width, height;
    u32 format;
    const void* data;
} GXTexObj;

// GX Render Mode Object (display settings)
typedef struct {
    u16 viWidth;
    u16 viHeight;
    u16 fbWidth;
    u16 efbHeight;
    u8 aa;
    u8 sample_pattern[12][2];
    u8 vfilter[7];
} GXRenderModeObj;

// ============================================================================
// GX Enumeration Definitions
// ============================================================================

// Primitive types
typedef enum {
    GX_POINTS = 0x0,
    GX_LINES = 0x1,
    GX_LINESTRIP = 0x2,
    GX_TRIANGLES = 0x3,
    GX_TRIANGLESTRIP = 0x4,
    GX_TRIANGLEFAN = 0x5,
    GX_QUADS = 0x6,
} GXPrimitive;

// Vertex format
typedef enum {
    GX_VTXFMT0 = 0,
    GX_VTXFMT1 = 1,
    GX_VTXFMT2 = 2,
    GX_VTXFMT3 = 3,
    GX_VTXFMT4 = 4,
    GX_VTXFMT5 = 5,
    GX_VTXFMT6 = 6,
    GX_VTXFMT7 = 7,
    GX_MAXVTXFMT = 8
} GXVtxFmt;

// Texture formats (GameCube)
typedef enum {
    GX_TF_I4 = 0x0,
    GX_TF_I8 = 0x1,
    GX_TF_IA4 = 0x2,
    GX_TF_IA8 = 0x3,
    GX_TF_RGB565 = 0x4,
    GX_TF_RGB5A3 = 0x5,
    GX_TF_RGBA8 = 0x6,
    GX_TF_CI4 = 0x8,
    GX_TF_CI8 = 0x9,
    GX_TF_CI14X2 = 0xA,
    GX_TF_CMPR = 0xE
} GXTexFmt;

// Texture wrap modes
typedef enum {
    GX_CLAMP = 0,
    GX_REPEAT = 1,
    GX_MIRROR = 2
} GXTexWrapMode;

// Texture filter modes
typedef enum {
    GX_NEAR = 0,
    GX_LINEAR = 1,
    GX_NEAR_MIP_NEAR = 2,
    GX_LIN_MIP_NEAR = 3,
    GX_NEAR_MIP_LIN = 4,
    GX_LIN_MIP_LIN = 5
} GXTexFilter;

// Boolean type
typedef enum {
    GX_FALSE = 0,
    GX_TRUE = 1
} GXBool;

// Anisotropy
typedef enum {
    GX_ANISO_1 = 0,
    GX_ANISO_2 = 1,
    GX_ANISO_4 = 2,
    GX_MAX_ANISOTROPY = 3
} GXAnisotropy;

// ============================================================================
// GX Wrapper State Management
// ============================================================================

typedef struct {
    GXPrimitive current_primitive;
    GLint current_texture;
    BOOL in_begin_end;
    u32 vertex_count;
    u32 max_vertex_count; // Expected vertex count for bounds checking
    
    // Current vertex data (for immediate mode simulation)
    f32 current_pos[3];
    f32 current_texcoord[2];
    GXColor current_color;
    
    // Matrices
    f32 modelview_matrix[16];
    f32 projection_matrix[16];
    
    // Display list support
    u8* display_list_buffer;
    u32 display_list_size;
    BOOL recording_display_list;
    
} GXWrapperState;

// Global wrapper state
extern GXWrapperState g_gx_state;

// ============================================================================
// Core GX → VitaGL Function Mappings
// ============================================================================

// --- Initialization and Setup ---
void GXInit(void* base, u32 size);
void GXSetVtxDesc(u32 attr, u32 type);
void GXSetVtxAttrFmt(GXVtxFmt vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
void GXClearVtxDesc(void);

// --- Primitive Rendering ---
void GXBegin(GXPrimitive type, GXVtxFmt vtxfmt, u16 nverts);
void GXEnd(void);

// --- Vertex Specification (Immediate Mode) ---
void GXPosition3f32(f32 x, f32 y, f32 z);
void GXPosition2f32(f32 x, f32 y);
void GXPosition3s16(s16 x, s16 y, s16 z);
void GXPosition2s16(s16 x, s16 y);
void GXPosition3u16(u16 x, u16 y, u16 z);
void GXPosition2u16(u16 x, u16 y);
void GXPosition3s8(s8 x, s8 y, s8 z);
void GXPosition2s8(s8 x, s8 y);

void GXTexCoord2f32(f32 s, f32 t);
void GXTexCoord2s16(s16 s, s16 t);
void GXTexCoord2u16(u16 s, u16 t);
void GXTexCoord2s8(s8 s, s8 t);
void GXTexCoord2u8(u8 s, u8 t);

void GXColor3f32(f32 r, f32 g, f32 b);
void GXColor4f32(f32 r, f32 g, f32 b, f32 a);
void GXColor3u8(u8 r, u8 g, u8 b);
void GXColor4u8(u8 r, u8 g, u8 b, u8 a);

void GXNormal3f32(f32 x, f32 y, f32 z);
void GXNormal3s16(s16 x, s16 y, s16 z);
void GXNormal3s8(s8 x, s8 y, s8 z);

// --- Texture Management ---
void GXInitTexObj(GXTexObj* obj, const void* data, u16 width, u16 height, 
                  GXTexFmt format, GXTexWrapMode wrap_s, GXTexWrapMode wrap_t, GXBool mipmap);
void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter min_filt, GXTexFilter mag_filt,
                     f32 min_lod, f32 max_lod, f32 lod_bias, GXBool bias_clamp,
                     GXBool do_edge_lod, GXAnisotropy max_aniso);
void GXLoadTexObj(GXTexObj* obj, u32 mapid);
void GXInvalidateTexAll(void);

// --- Display Lists ---
void GXBeginDisplayList(void* list, u32 size);
u32 GXEndDisplayList(void);
void GXCallDisplayList(const void* list, u32 nbytes);

// --- Matrix Operations ---
void GXLoadIdentity(void);
void GXLoadMatrixf(const f32 matrix[16]);
void GXLoadProjectionMtx(const f32 matrix[16], GXBool ortho);
void GXSetCurrentMtx(u32 mtx);

// --- Viewport and Scissor ---
void GXSetViewport(f32 xOrig, f32 yOrig, f32 wd, f32 ht, f32 nearZ, f32 farZ);
void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht);

// --- Frame Buffer ---
void GXSetColorUpdate(GXBool update_enable);
void GXSetAlphaUpdate(GXBool update_enable);
void GXCopyDisp(void* dest, GXBool clear);
void GXDrawDone(void);
void GXPixModeSync(void);

// --- Utility Functions ---
void GXFlush(void);
void GXFinish(void);

// ============================================================================
// VitaGL Helper Functions
// ============================================================================

// Convert GX primitive to OpenGL primitive
GLenum gx_primitive_to_gl(GXPrimitive prim);

// Convert GX texture format to OpenGL format
void gx_format_to_gl(GXTexFmt gx_format, GLenum* gl_format, GLenum* gl_type);

// Convert GX wrap mode to OpenGL wrap mode
GLenum gx_wrap_to_gl(GXTexWrapMode wrap);

// Convert GX filter to OpenGL filter
GLenum gx_filter_to_gl(GXTexFilter filter);

// Matrix utilities
void gx_matrix_identity(f32 matrix[16]);
void gx_matrix_multiply(const f32 a[16], const f32 b[16], f32 result[16]);

// ============================================================================
// AC-Specific Helper Functions
// ============================================================================

// Integration with our existing texture loading system
GLuint gx_load_ac_texture(const char* asset_name, u16* width, u16* height);

// Character rendering helpers (for AC's modular character system)
void gx_render_character_part(const char* texture_name, f32 x, f32 y, f32 scale);

// World rendering helpers
void gx_setup_ac_camera(f32 eye_x, f32 eye_y, f32 eye_z, 
                        f32 target_x, f32 target_y, f32 target_z);

// ============================================================================
// Debug and Logging
// ============================================================================

// Enable/disable GX call logging
void GXSetDebugMode(BOOL enable);

// Print current GX state
void GXPrintState(void);

#ifdef __cplusplus
}
#endif

#endif // GX_TO_VITAGL_WRAPPER_H 