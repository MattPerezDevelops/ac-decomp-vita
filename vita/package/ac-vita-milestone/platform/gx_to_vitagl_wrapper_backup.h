// EARLY HEADER BLOCKING - Must be first
#define LIBULTRA_LIBULTRA_H_INCLUDED 1
#define LIBC64_MATH64_H_INCLUDED 1


#ifndef GX_TO_VITAGL_WRAPPER_H
#define GX_TO_VITAGL_WRAPPER_H

/**
 * Comprehensive GameCube GX → VitaGL Graphics Wrapper
 * 
 * This header provides a complete translation layer between GameCube's GX graphics API
 * and PlayStation Vita's VitaGL (OpenGL ES 1.1) implementation.
 * 
 * AUTO-INCLUDED in all AC-decomp files for mass conversion.
 * Our VitaGL implementation takes precedence over GameCube headers.
 * 
 * Architecture: AC-decomp → GameCube API → VitaGL OpenGL 1.x → SceGxm
 */

// ============================================================================
// Header Conflict Prevention (Phase 3)
// ============================================================================

// Block problematic GameCube headers from being included after us
#ifndef AC_VITAGL_HEADER_GUARD
#define AC_VITAGL_HEADER_GUARD

// Prevent GameCube SDK headers that conflict with our implementation
#define LIBULTRA_LIBULTRA_H_INCLUDED  
#define DOLPHIN_GX_H_INCLUDED
#define MSL_MATH_H_INCLUDED

// Block individual GameCube header components
#define __DOLPHIN_GX_GXENUM_H__
#define __DOLPHIN_GX_GXSTRUCT_H__ 
#define __DOLPHIN_GX_GXCULL_H__
#define __DOLPHIN_GX_GXDISPLIST_H__
#define __DOLPHIN_GX_GXGEOMETRY_H__
#define __DOLPHIN_GX_GXMANAGE_H__
#define __DOLPHIN_GX_GXTEXTURE_H__
#define __DOLPHIN_GX_GXTRANSFORM_H__
#define __DOLPHIN_GX_GXVERT_H__
#define __LIBULTRA_ULTRATYPES_H__
#define __LIBULTRA_GU_H__
#define __DOLPHIN_MTX_H__
#define __SYS_MATRIX_H__

// Our VitaGL definitions take precedence
#define GX_TYPES_DEFINED_BY_VITAGL
#define GFX_TYPE_DEFINED_BY_VITAGL
#define MTX_TYPE_DEFINED_BY_VITAGL

// Prevent GameCube/N64 string functions from conflicting with VitaSDK

#endif // AC_VITAGL_HEADER_GUARD

// ============================================================================
// VitaGL Core (Phase 1)
// ============================================================================

#include <vitaGL.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Prevent redefinition warnings by defining our constants first
#ifndef G_MTX_NOPUSH
#define G_MTX_NOPUSH 0x01
#define G_MTX_LOAD 0x02
#define G_MTX_MODELVIEW 0x00
#define G_CULL_BACK 0x1000
#define G_CYC_1CYCLE 0
#define G_IM_FMT_I 6
#endif

// Matrix constants that AC-decomp expects
#ifndef MTX_LOAD
#define MTX_LOAD 0  // load into new matrix
#define MTX_MULT 1  // multiply into existing matrix
#endif

// GameCube/N64 type definitions (our constants take precedence)
// Note: PR/gbi.h only included when AC-decomp is available
#ifdef AC_DECOMP_INTEGRATED
#include "PR/gbi.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GX Type Definitions (Essential GameCube Types)
// ============================================================================

// Basic types first (needed by other types)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef float f32;

// Matrix types (now that f32 is defined)
typedef f32 Mtx[4][4];
typedef f32 MtxF[4][4];
typedef f32 (*MtxP)[4];  // AC-decomp: Array pointer, not simple pointer

// Graphics command type (GameCube display list entry)
typedef struct {
    u32 words[2];
} Gfx;

// Vertex type (N64/GameCube vertex structure)
typedef struct {
    s16 x, y, z;        // Position
    u16 flag;           // Flags  
    s16 tc[2];          // Texture coordinates
    u8 cn[4];           // Color and normal
} Vtx;

// Viewport type (GameCube viewport structure)
typedef struct {
    u16 vscale[4];      // Viewport scale
    u16 vtrans[4];      // Viewport translation  
} Vp;

// Lighting types (GameCube lighting structures)
typedef struct {
    u8 r, g, b, a;      // Ambient light color
} Ambient;

typedef struct {
    u8 r, g, b, a;      // Light color
    s8 x, y, z;         // Light direction
} Light_t;

typedef struct {
    f32 m[4][4];        // LookAt matrix
} LookAt;

typedef struct {
    u8 r, g, b, a;      // Hilite color  
} Hilite;

// Graphics setimg structure
typedef struct {
    u32 cmd;            // Command
    void* imgptr;       // Image pointer
} Gsetimg;

// GRAPH type (AC-decomp expects struct graph_s)
typedef struct graph_s {
    Gfx* Gfx_list00;        // polygon opaque  
    Gfx* Gfx_list01;        // polygon translucent
    void* DepthBuffer;      // depth buffer
    Gfx* Gfx_list03;        // unused
    Gfx* Gfx_list04;        // overlay
    Gfx* Gfx_list07;        // font
    // Simplified structure - AC-decomp compatible
    u8 rest[0x350];         // Rest of the structure
} GRAPH;

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
// GameCube Graphics Functions (N64/GC compatibility)
// ============================================================================

// Display list and matrix functions AC-decomp uses (original GameCube signatures)
void gSPMatrix(Gfx* gfx, Mtx* m, u32 flags);
void gSPDisplayList(Gfx* gfx, const Gfx* dl);
void gDPSetPrimColor(Gfx* gfx, u32 minlevel, u32 maxlevel, u8 r, u8 g, u8 b, u8 a);
void gDPSetEnvColor(Gfx* gfx, u8 r, u8 g, u8 b, u8 a);
void gDPSetOtherMode(Gfx* gfx, u32 mode1, u32 mode2);
void gSPLoadGeometryMode(Gfx* gfx, u32 mode);
// NOTE: gDPSetCombineMode is a macro in GameCube, not a function
#define gDPSetCombineMode(gfx, mode1, mode2) do { \
    /* TODO: Set OpenGL combine mode based on mode1/mode2 */ \
} while(0)
void gDPLoadTextureTile(Gfx* gfx, void* timg, u32 fmt, u32 siz,
                        u32 width, u32 height, u32 uls, u32 ult, u32 lrs, u32 lrt,
                        u32 pal, u32 cms, u32 cmt, u32 masks, u32 maskt,
                        u32 shifts, u32 shiftt);
void gSPTexture(Gfx* gfx, u16 sc, u16 tc, s32 level, s32 tile, s32 on);

// HIGH PRIORITY missing functions (used 50+ times in AC-decomp)
void gDPPipeSync(Gfx* gfx);
void gDPFullSync(Gfx* gfx);
void gSPSegment(Gfx* gfx, u32 segment, void* base);
void gSPBranchList(Gfx* gfx, Gfx* dl);

// MEDIUM PRIORITY missing functions (used 20+ times)
void gSPLoadUcode(Gfx* gfx, void* uc_start, void* uc_dstart);
void gDPLoadTLUT(Gfx* gfx, u32 count, u32 tmem_addr, void* tlut);
void gSPObjRenderMode(Gfx* gfx, u32 mode);
void gDPSetCombineLERP(Gfx* gfx, u32 a0, u32 b0, u32 c0, u32 d0, u32 aa0, u32 ab0, u32 ac0, u32 ad0,
                       u32 a1, u32 b1, u32 c1, u32 d1, u32 aa1, u32 ab1, u32 ac1, u32 ad1);
void gDPSetColorImage(Gfx* gfx, u32 fmt, u32 siz, u32 width, void* img);
void gDPSetScissor(Gfx* gfx, u32 mode, u32 ulx, u32 uly, u32 lrx, u32 lry);
void gDPFillRectangle(Gfx* gfx, u32 ulx, u32 uly, u32 lrx, u32 lry);
void gSPEndDisplayList(Gfx* gfx);

// Background and object rendering
void gSPBgRectCopy(Gfx* gfx, void* bg);
void gSPBgRect1Cyc(Gfx* gfx, void* bg);
void gDPSetBlendColor(Gfx* gfx, u8 r, u8 g, u8 b, u8 a);
void gDPSetPrimDepth(Gfx* gfx, u16 z, u16 dz);

// Static versions (gs prefixed)
#define gsSPMatrix(m, flags) (Gfx){.words = {0, 0}} // Simplified
#define gsSPDisplayList(dl) (Gfx){.words = {0, 0}}
#define gsSPLoadGeometryMode(mode) (Gfx){.words = {0, 0}}
#define gsDPSetOtherMode(mode1, mode2) (Gfx){.words = {0, 0}}
#define gsDPSetCombineMode(mode1, mode2) (Gfx){.words = {0, 0}}
#define gsSPTexture(sc, tc, level, tile, on) (Gfx){.words = {0, 0}}
#define gsSPEndDisplayList() (Gfx){.words = {0, 0}}

// GameCube constants that AC-decomp expects (avoid duplicates)
#ifndef G_ON
#define G_ON 1
#define G_OFF 0
#endif

// Color combiner constants (used in gDPSetCombineMode calls)
#define G_CC_TITLE          0x00, 0x00
#define G_CC_PRESS_START    0x01, 0x01  
#define G_CC_PRIMITIVE      0x02, 0x02

// Texture constants
#ifndef G_TX_WRAP
#define G_TX_WRAP 0
#define G_TX_NOMIRROR 0
#define G_TX_NOMASK 0
#define G_TX_NOLOD 0
#endif

// Display List Management constants
#define G_MWO_SEGMENT_0 0x00
#define G_MWO_NUMLIGHT 0x88

// Image formats
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_I 6
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_8b 1

// Scissor modes
#define G_SC_NON_INTERLACE 0

// Cycle types (avoid duplicates)
#ifndef G_CYC_2CYCLE
#define G_CYC_2CYCLE 1
#define G_CYC_COPY 2
#define G_CYC_FILL 3
#endif

// Alpha dither
#define G_AD_DISABLE 0x0002
#define G_AD_NOTPATTERN 0x0000

// Color dither  
#define G_CD_DISABLE 0x0004
#define G_CD_MAGICSQ 0x0000

// Object render modes
#define G_OBJRM_ANTIALIAS 0x01
#define G_OBJRM_BILERP 0x02

// Color combiner constants  
#define PRIMITIVE 0
#define ENVIRONMENT 1
#define TEXEL0 2
#define COMBINED 3

// Advanced combiner constants
#define G_TC_FILT 0x2000
#define G_TF_POINT 0x0000
#define G_TF_BILERP 0x0800
#define G_TT_NONE 0x0000
#define G_TT_RGBA16 0x8000
#define G_TL_TILE 0x0000
#define G_TD_CLAMP 0x0000
#define G_TP_PERSP 0x0080
#define G_TP_NONE 0x0000
#define G_PM_NPRIMITIVE 0x0000

// Alpha compare
#define G_AC_NONE 0x0000
#define G_AC_THRESHOLD 0x0001

// Z source
#define G_ZS_PRIM 0x0400

// Render modes
#define G_RM_XLU_SURF 0x0C08
#define G_RM_XLU_SURF2 0x0302
#define G_RM_CLD_SURF 0x0504  
#define G_RM_CLD_SURF2 0x0145

// Matrix helper functions that AC-decomp expects
// AC-decomp expects these exact signatures (from sys_matrix.h)
Mtx* _Matrix_to_Mtx_new(GRAPH* graph);  // AC-decomp: GRAPH*, not void*
Mtx* _MtxF_to_Mtx(MtxF* src, Mtx* dest); // AC-decomp: Mtx* return, not void
void Matrix_MtxtoMtxF(Mtx* src, MtxF* dest);

// Additional matrix functions that AC-decomp expects
void Matrix_push(void);
void Matrix_pull(void);
void Matrix_translate(f32 x, f32 y, f32 z, u8 flag);

// Additional graphics helper functions
Gfx* gfx_gSPTextureRectangle1(Gfx* gfx, int xl, int yl, int xh, int yh, int tile, int s, int t, int dsdx, int dtdy);

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


// Prevent GRAPH redefinition conflicts
#ifndef GRAPH_TYPE_DEFINED_BY_VITAGL
#define GRAPH_TYPE_DEFINED_BY_VITAGL 1
#endif

// Enhanced type guards to prevent conflicts
#ifndef VITA_TYPES_FULLY_DEFINED
#define VITA_TYPES_FULLY_DEFINED 1
#define __U32_DEFINED 1
#define __S32_DEFINED 1
#endif

// GameCube OS Message types (with conflict prevention)
#ifndef OSMESSAGE_TYPES_DEFINED_BY_VITAGL
#define OSMESSAGE_TYPES_DEFINED_BY_VITAGL 1

#ifndef OSMessage
typedef struct {
    void* msg;
    u32 msgqueueid;
} OSMessage;
#endif

#ifndef OSMessageQueue  
typedef struct {
    OSMessage* msgBuf;
    s32 msgCount; 
    s32 first;
    s32 used;
    void* mtqueue;
    void* mfullqueue;
} OSMessageQueue;
#endif

// Alternative spellings (also guarded)
#ifndef OSMesg
typedef OSMessage OSMesg;
#endif

#ifndef OSMesgQueue
typedef OSMessageQueue OSMesgQueue;
#endif

#endif // OSMESSAGE_TYPES_DEFINED_BY_VITAGL

// Complete libultra header blocking
#ifndef LIBULTRA_COMPLETELY_BLOCKED
#define LIBULTRA_COMPLETELY_BLOCKED 1
#define LIBULTRA_LIBULTRA_H_INCLUDED 1  
#define __LIBULTRA_LIBULTRA_H__ 1
#define _LIBULTRA_H_ 1
#define LIBULTRA_H 1
#endif
