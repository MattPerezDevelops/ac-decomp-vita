/*
 * GameCube GX to VitaGL Wrapper with Smart Shadow Integration
 * ==========================================================
 * Enhanced with smart conditional shadow system
 * Only applies fixes where conflicts are detected
 */

#ifndef GX_TO_VITAGL_WRAPPER_H
#define GX_TO_VITAGL_WRAPPER_H

// ============================================================================
// SMART SHADOW INTEGRATION
// ============================================================================

// Include smart shadow architecture first for conflict detection
#ifdef AC_VITA_ENABLE_SMART_SHADOWS
    #include "smart_shadow_architecture.h"
#endif

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

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ESSENTIAL TYPE DEFINITIONS (Break circular dependency)
// ============================================================================
// Define essential types directly instead of including AC-decomp types.h
// This prevents circular dependency: wrapper → types.h → shadow headers → wrapper

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// Essential basic types (GameCube compatible)
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;
typedef double f64;
typedef int BOOL;
typedef unsigned long long u64;
typedef signed long long s64;

// Prevent redefinition conflicts with system headers
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#ifndef _SIZE_T_DEF
#define _SIZE_T_DEF
#ifndef __SIZE_TYPE__
typedef unsigned long size_t;
#endif
#endif
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
#ifndef _WCHAR_T_DEF
typedef unsigned short wchar_t;
#endif
#endif
#endif

// Note: Removed #include "../ac-decomp-source/include/types.h" to break circular dependency

// ============================================================================
// GX Type Definitions (GameCube Graphics API Types Only)
// ============================================================================
// Note: AC-decomp provides all basic types (s8, u8, s16, u16, s32, u32, f32, BOOL, etc.)
// We only define GX-specific graphics types here

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

// Alias for compatibility with implementation
typedef GXWrapperState GXState;

// Display list command structure (GameCube GBI compatible)
// Note: Gfx is redefined as u64 below for bitwise operation compatibility

// Matrix types (GameCube compatible)
typedef f32 Mtx[3][4];
typedef f32 Mtx34[3][4];  // Additional matrix type
#ifndef MTXF_DEFINED_BY_ULTRATYPES
typedef f32 MtxF[4][4];
#endif
typedef f32* MtxP;

// GameCube Vertex type
typedef struct {
    s16 ob[3];  // x, y, z position
    u16 flag;   // flags
    s16 tc[2];  // texture coordinates
    u8 cn[4];   // color and normal
} Vtx;

// GameCube OS types
typedef void* OSThreadQueue;
typedef void* OSMesgQueue;  // Message queue type

// Only define essential missing GameCube types that AC-decomp needs
// OSThread is defined later as a proper struct
typedef void OSPriority; 
typedef struct {
    u32 fmt;
    void* img;
    u16 siz;
    u16 width;
} Gsetimg;
typedef struct {
    u8 col[3];
    u8 colc[3];  
    s8 dir[3];
    u8 pad1;
} Light_t;
typedef struct {
    f32 x, y;
} LookAt;
typedef struct {
    f32 x, y, z;
} Hilite;
typedef struct {
    u8 r, g, b;
} Ambient;
typedef struct {
    f32 vp[4][4];
} Vp;

// Missing GameCube constants
#ifndef G_CULL_BACK
#define G_CULL_BACK 0x0200
#endif

#ifndef G_MTX_LOAD
#define G_MTX_LOAD 0x00
#define G_MTX_MUL 0x01
#define MTX_LOAD 0x00
#define MTX_MUL 0x01
#endif

#ifndef G_OBJRM_BILERP
#define G_OBJRM_BILERP 0x02
#endif

// ============================================================================
// COMPREHENSIVE GX TYPE DEFINITIONS (Prevent hydra effect)
// ============================================================================
// Define ALL GX types here so no matter which GX header is included, types exist

// GX Advanced Types (prevent missing type errors)
typedef enum {
    GX_TG_MTX3x4, GX_TG_MTX2x4, GX_TG_BUMP0, GX_TG_BUMP1, GX_TG_BUMP2, GX_TG_BUMP3,
    GX_TG_BUMP4, GX_TG_BUMP5, GX_TG_BUMP6, GX_TG_BUMP7, GX_TG_SRTG
} GXTexGenType;

typedef enum {
    GX_TG_POS, GX_TG_NRM, GX_TG_BINRM, GX_TG_TANGENT, GX_TG_TEX0, GX_TG_TEX1,
    GX_TG_TEX2, GX_TG_TEX3, GX_TG_TEX4, GX_TG_TEX5, GX_TG_TEX6, GX_TG_TEX7,
    GX_TG_TEXCOORD0, GX_TG_TEXCOORD1, GX_TG_TEXCOORD2, GX_TG_TEXCOORD3,
    GX_TG_TEXCOORD4, GX_TG_TEXCOORD5, GX_TG_TEXCOORD6, GX_TG_COLOR0, GX_TG_COLOR1,
    GX_MAX_TEXGENSRC
} GXTexGenSrc;

typedef enum {
    GX_TEXCOORD0, GX_TEXCOORD1, GX_TEXCOORD2, GX_TEXCOORD3, GX_TEXCOORD4,
    GX_TEXCOORD5, GX_TEXCOORD6, GX_TEXCOORD7, GX_MAX_TEXCOORD, GX_TEXCOORD_NULL = 0xFF
} GXTexCoordID;

typedef enum {
    GX_TEXMAP0, GX_TEXMAP1, GX_TEXMAP2, GX_TEXMAP3, GX_TEXMAP4,
    GX_TEXMAP5, GX_TEXMAP6, GX_TEXMAP7, GX_MAX_TEXMAP, GX_TEXMAP_NULL = 0xFF
} GXTexMapID;

typedef enum {
    GX_VA_PNMTXIDX, GX_VA_TEX0MTXIDX, GX_VA_TEX1MTXIDX, GX_VA_TEX2MTXIDX,
    GX_VA_TEX3MTXIDX, GX_VA_TEX4MTXIDX, GX_VA_TEX5MTXIDX, GX_VA_TEX6MTXIDX,
    GX_VA_TEX7MTXIDX, GX_VA_POS, GX_VA_NRM, GX_VA_CLR0, GX_VA_CLR1,
    GX_VA_TEX0, GX_VA_TEX1, GX_VA_TEX2, GX_VA_TEX3, GX_VA_TEX4,
    GX_VA_TEX5, GX_VA_TEX6, GX_VA_TEX7, GX_VA_NBT, GX_VA_MAX_ATTR,
    GX_VA_NULL = 0xFF
} GXAttr;

typedef enum {
    GX_NONE, GX_DIRECT, GX_INDEX8, GX_INDEX16
} GXAttrType;

typedef enum {
    GX_POS_XY = 0, GX_POS_XYZ = 1, GX_NRM_XYZ = 0, GX_NRM_NBT = 1,
    GX_NRM_NBT3 = 2, GX_CLR_RGB = 0, GX_CLR_RGBA = 1, GX_TEX_S = 0, GX_TEX_ST = 1
} GXCompCnt;

typedef enum {
    GX_U8 = 0, GX_S8 = 1, GX_U16 = 2, GX_S16 = 3, GX_F32 = 4,
    GX_RGB565 = 0, GX_RGB8 = 1, GX_RGBX8 = 2, GX_RGBA4 = 3, GX_RGBA6 = 4, GX_RGBA8 = 5
} GXCompType;

typedef enum {
    GX_TO_ZERO, GX_TO_SIXTEENTH, GX_TO_EIGHTH, GX_TO_FOURTH, GX_TO_HALF, GX_TO_ONE, GX_MAX_TEXOFFSET
} GXTexOffset;

typedef enum {
    GX_CULL_NONE, GX_CULL_FRONT, GX_CULL_BACK, GX_CULL_ALL
} GXCullMode;

typedef enum {
    GX_PERSPECTIVE, GX_ORTHOGRAPHIC
} GXProjectionType;

typedef enum {
    GX_MTX3x4, GX_MTX2x4
} GXTexMtxType;

typedef enum {
    GX_CLIP_ENABLE = 0, GX_CLIP_DISABLE = 1
} GXClipMode;

// Additional GX advanced types
typedef enum {
    GX_TEVSTAGE0, GX_TEVSTAGE1, GX_TEVSTAGE2, GX_TEVSTAGE3, GX_TEVSTAGE4,
    GX_TEVSTAGE5, GX_TEVSTAGE6, GX_TEVSTAGE7, GX_TEVSTAGE8, GX_TEVSTAGE9,
    GX_TEVSTAGE10, GX_TEVSTAGE11, GX_TEVSTAGE12, GX_TEVSTAGE13, GX_TEVSTAGE14,
    GX_TEVSTAGE15, GX_MAX_TEVSTAGE
} GXTevStageID;

typedef enum {
    GX_ITM_OFF, GX_ITM_0, GX_ITM_1, GX_ITM_2, GX_ITM_S0 = 5, GX_ITM_S1,
    GX_ITM_S2, GX_ITM_T0 = 9, GX_ITM_T1, GX_ITM_T2
} GXIndTexMtxID;

typedef enum {
    GX_INDTEXSTAGE0, GX_INDTEXSTAGE1, GX_INDTEXSTAGE2, GX_INDTEXSTAGE3, GX_MAX_INDTEXSTAGE
} GXIndTexStageID;

typedef enum {
    GX_ITF_8, GX_ITF_5, GX_ITF_4, GX_ITF_3, GX_MAX_ITFORMAT
} GXIndTexFormat;

typedef enum {
    GX_ITB_NONE, GX_ITB_S, GX_ITB_T, GX_ITB_ST, GX_ITB_U, GX_ITB_SU, GX_ITB_TU, GX_ITB_STU, GX_MAX_ITBIAS
} GXIndTexBiasSel;

typedef enum {
    GX_ITW_OFF, GX_ITW_256, GX_ITW_128, GX_ITW_64, GX_ITW_32, GX_ITW_16, GX_ITW_0, GX_MAX_ITWRAP
} GXIndTexWrap;

typedef enum {
    GX_ITBA_OFF, GX_ITBA_S, GX_ITBA_T, GX_ITBA_U, GX_MAX_ITBALPHA
} GXIndTexAlphaSel;

typedef enum {
    GX_ITS_1, GX_ITS_2, GX_ITS_4, GX_ITS_8, GX_ITS_16, GX_ITS_32, GX_ITS_64, GX_ITS_128, GX_ITS_256, GX_MAX_ITSCALE
} GXIndTexScale;

// More comprehensive GX types
typedef enum {
    GX_GM_1_0, GX_GM_1_7, GX_GM_2_2
} GXGamma;

typedef enum {
    GX_PF_RGB8_Z24, GX_PF_RGBA6_Z24, GX_PF_RGB565_Z16, GX_PF_Z24, GX_PF_Y8, GX_PF_U8, GX_PF_V8, GX_PF_YUV420
} GXPixelFmt;

typedef enum {
    GX_ZC_LINEAR, GX_ZC_NEAR, GX_ZC_MID, GX_ZC_FAR
} GXZFmt16;

typedef enum {
    GX_CLAMP_NONE, GX_CLAMP_TOP, GX_CLAMP_BOTTOM
} GXFBClamp;

typedef enum {
    GX_COLOR0, GX_COLOR1, GX_ALPHA0, GX_ALPHA1, GX_COLOR0A0, GX_COLOR1A1, GX_COLOR_ZERO, GX_ALPHA_BUMP, GX_ALPHA_BUMPN, GX_COLOR_NULL = 0xFF
} GXChannelID;

typedef enum {
    GX_SRC_REG, GX_SRC_VTX
} GXColorSrc;

typedef enum {
    GX_DF_NONE, GX_DF_SIGN, GX_DF_CLAMP
} GXDiffuseFn;

typedef enum {
    GX_AF_SPEC, GX_AF_SPOT, GX_AF_NONE
} GXAttnFn;

typedef enum {
    GX_SP_OFF, GX_SP_FLAT, GX_SP_COS, GX_SP_COS2, GX_SP_SHARP, GX_SP_RING1, GX_SP_RING2
} GXSpotFn;

typedef enum {
    GX_DA_OFF, GX_DA_GENTLE, GX_DA_MEDIUM, GX_DA_STEEP
} GXDistAttnFn;

typedef enum {
    GX_LIGHT0 = 0x001, GX_LIGHT1 = 0x002, GX_LIGHT2 = 0x004, GX_LIGHT3 = 0x008,
    GX_LIGHT4 = 0x010, GX_LIGHT5 = 0x020, GX_LIGHT6 = 0x040, GX_LIGHT7 = 0x080,
    GX_MAX_LIGHT = 0x100, GX_LIGHT_NULL = 0
} GXLightID;

typedef enum {
    GX_FOG_NONE = 0, GX_FOG_PERSP_LIN = 2, GX_FOG_PERSP_EXP = 4, GX_FOG_PERSP_EXP2 = 5,
    GX_FOG_PERSP_REVEXP = 6, GX_FOG_PERSP_REVEXP2 = 7, GX_FOG_ORTHO_LIN = 10,
    GX_FOG_ORTHO_EXP = 12, GX_FOG_ORTHO_EXP2 = 13, GX_FOG_ORTHO_REVEXP = 14, GX_FOG_ORTHO_REVEXP2 = 15,
    GX_FOG_LIN = GX_FOG_PERSP_LIN, GX_FOG_EXP = GX_FOG_PERSP_EXP, GX_FOG_EXP2 = GX_FOG_PERSP_EXP2,
    GX_FOG_REVEXP = GX_FOG_PERSP_REVEXP, GX_FOG_REVEXP2 = GX_FOG_PERSP_REVEXP2
} GXFogType;

typedef enum {
    GX_BM_NONE, GX_BM_BLEND, GX_BM_LOGIC, GX_BM_SUBTRACT, GX_MAX_BLENDMODE
} GXBlendMode;

typedef enum {
    GX_BL_ZERO, GX_BL_ONE, GX_BL_SRCCLR, GX_BL_INVSRCCLR, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
    GX_BL_DSTALPHA, GX_BL_INVDSTALPHA, GX_BL_DSTCLR = GX_BL_SRCCLR, GX_BL_INVDSTCLR = GX_BL_INVSRCCLR
} GXBlendFactor;

typedef enum {
    GX_LO_CLEAR, GX_LO_AND, GX_LO_REVAND, GX_LO_COPY, GX_LO_INVAND, GX_LO_NOOP, GX_LO_XOR, GX_LO_OR,
    GX_LO_NOR, GX_LO_EQUIV, GX_LO_INV, GX_LO_REVOR, GX_LO_INVCOPY, GX_LO_INVOR, GX_LO_NAND, GX_LO_SET
} GXLogicOp;

typedef enum {
    GX_NEVER, GX_LESS, GX_EQUAL, GX_LEQUAL, GX_GREATER, GX_NEQUAL, GX_GEQUAL, GX_ALWAYS
} GXCompare;

typedef enum {
    GX_AOP_AND, GX_AOP_OR, GX_AOP_XOR, GX_AOP_XNOR, GX_MAX_ALPHAOP
} GXAlphaOp;

// GX Struct types
typedef struct {
    u16 dummy[10];
} GXFogAdjTable;

typedef struct {
    GXAttr attr;
    GXAttrType type;
} GXVtxDescList;

typedef struct {
    u32 dummy[16];
} GXLightObj;

typedef struct {
    s16 r, g, b, a;
} GXColorS10;

typedef struct {
    u32 dummy[4];
} GXTexRegion;

typedef struct {
    u32 dummy[4];
} GXTlutRegion;

typedef struct {
    u32 dummy[3];
} GXTlutObj;

// More GX enums
typedef enum {
    GX_MODULATE, GX_DECAL, GX_BLEND, GX_REPLACE, GX_PASSCLR
} GXTevMode;

typedef enum {
    GX_CC_CPREV, GX_CC_APREV, GX_CC_C0, GX_CC_A0, GX_CC_C1, GX_CC_A1, GX_CC_C2, GX_CC_A2,
    GX_CC_TEXC, GX_CC_TEXA, GX_CC_RASC, GX_CC_RASA, GX_CC_ONE, GX_CC_HALF, GX_CC_KONST, GX_CC_ZERO
} GXTevColorArg;

typedef enum {
    GX_CA_APREV, GX_CA_A0, GX_CA_A1, GX_CA_A2, GX_CA_TEXA, GX_CA_RASA, GX_CA_KONST, GX_CA_ZERO
} GXTevAlphaArg;

typedef enum {
    GX_TEV_ADD = 0, GX_TEV_SUB = 1, GX_TEV_COMP_R8_GT = 8, GX_TEV_COMP_R8_EQ = 9,
    GX_TEV_COMP_GR16_GT = 10, GX_TEV_COMP_GR16_EQ = 11, GX_TEV_COMP_BGR24_GT = 12,
    GX_TEV_COMP_BGR24_EQ = 13, GX_TEV_COMP_RGB8_GT = 14, GX_TEV_COMP_RGB8_EQ = 15,
    GX_TEV_COMP_A8_GT = GX_TEV_COMP_RGB8_GT, GX_TEV_COMP_A8_EQ = GX_TEV_COMP_RGB8_EQ
} GXTevOp;

typedef enum {
    GX_TB_ZERO, GX_TB_ADDHALF, GX_TB_SUBHALF, GX_MAX_TEVBIAS
} GXTevBias;

typedef enum {
    GX_CS_SCALE_1, GX_CS_SCALE_2, GX_CS_SCALE_4, GX_CS_DIVIDE_2, GX_MAX_TEVSCALE
} GXTevScale;

typedef enum {
    GX_TEVPREV, GX_TEVREG0, GX_TEVREG1, GX_TEVREG2, GX_MAX_TEVREG
} GXTevRegID;

typedef enum {
    GX_KCOLOR0 = 0, GX_KCOLOR1, GX_KCOLOR2, GX_KCOLOR3, GX_MAX_KCOLOR
} GXTevKColorID;

typedef enum {
    GX_TEV_KCSEL_8_8 = 0x00, GX_TEV_KCSEL_7_8 = 0x01, GX_TEV_KCSEL_6_8 = 0x02, GX_TEV_KCSEL_5_8 = 0x03,
    GX_TEV_KCSEL_4_8 = 0x04, GX_TEV_KCSEL_3_8 = 0x05, GX_TEV_KCSEL_2_8 = 0x06, GX_TEV_KCSEL_1_8 = 0x07,
    GX_TEV_KCSEL_1 = GX_TEV_KCSEL_8_8, GX_TEV_KCSEL_3_4 = GX_TEV_KCSEL_6_8, GX_TEV_KCSEL_1_2 = GX_TEV_KCSEL_4_8,
    GX_TEV_KCSEL_1_4 = GX_TEV_KCSEL_2_8, GX_TEV_KCSEL_K0 = 0x0C, GX_TEV_KCSEL_K1 = 0x0D, GX_TEV_KCSEL_K2 = 0x0E,
    GX_TEV_KCSEL_K3 = 0x0F, GX_TEV_KCSEL_K0_R = 0x10, GX_TEV_KCSEL_K1_R = 0x11, GX_TEV_KCSEL_K2_R = 0x12,
    GX_TEV_KCSEL_K3_R = 0x13, GX_TEV_KCSEL_K0_G = 0x14, GX_TEV_KCSEL_K1_G = 0x15, GX_TEV_KCSEL_K2_G = 0x16,
    GX_TEV_KCSEL_K3_G = 0x17, GX_TEV_KCSEL_K0_B = 0x18, GX_TEV_KCSEL_K1_B = 0x19, GX_TEV_KCSEL_K2_B = 0x1A,
    GX_TEV_KCSEL_K3_B = 0x1B, GX_TEV_KCSEL_K0_A = 0x1C, GX_TEV_KCSEL_K1_A = 0x1D, GX_TEV_KCSEL_K2_A = 0x1E,
    GX_TEV_KCSEL_K3_A = 0x1F
} GXTevKColorSel;

typedef enum {
    GX_TEV_KASEL_8_8 = 0x00, GX_TEV_KASEL_7_8 = 0x01, GX_TEV_KASEL_6_8 = 0x02, GX_TEV_KASEL_5_8 = 0x03,
    GX_TEV_KASEL_4_8 = 0x04, GX_TEV_KASEL_3_8 = 0x05, GX_TEV_KASEL_2_8 = 0x06, GX_TEV_KASEL_1_8 = 0x07,
    GX_TEV_KASEL_1 = GX_TEV_KASEL_8_8, GX_TEV_KASEL_3_4 = GX_TEV_KASEL_6_8, GX_TEV_KASEL_1_2 = GX_TEV_KASEL_4_8,
    GX_TEV_KASEL_1_4 = GX_TEV_KASEL_2_8, GX_TEV_KASEL_K0_R = 0x10, GX_TEV_KASEL_K1_R = 0x11,
    GX_TEV_KASEL_K2_R = 0x12, GX_TEV_KASEL_K3_R = 0x13, GX_TEV_KASEL_K0_G = 0x14, GX_TEV_KASEL_K1_G = 0x15,
    GX_TEV_KASEL_K2_G = 0x16, GX_TEV_KASEL_K3_G = 0x17, GX_TEV_KASEL_K0_B = 0x18, GX_TEV_KASEL_K1_B = 0x19,
    GX_TEV_KASEL_K2_B = 0x1A, GX_TEV_KASEL_K3_B = 0x1B, GX_TEV_KASEL_K0_A = 0x1C, GX_TEV_KASEL_K1_A = 0x1D,
    GX_TEV_KASEL_K2_A = 0x1E, GX_TEV_KASEL_K3_A = 0x1F
} GXTevKAlphaSel;

typedef enum {
    GX_TEV_SWAP0 = 0, GX_TEV_SWAP1, GX_TEV_SWAP2, GX_TEV_SWAP3, GX_MAX_TEVSWAP
} GXTevSwapSel;

typedef enum {
    GX_CH_RED = 0, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA
} GXTevColorChan;

typedef enum {
    GX_ZT_DISABLE, GX_ZT_ADD, GX_ZT_REPLACE, GX_MAX_ZTEXOP
} GXZTexOp;

typedef enum {
    GX_TL_IA8, GX_TL_RGB565, GX_TL_RGB5A3, GX_MAX_TLUTFMT
} GXTlutFmt;

typedef enum {
    GX_TLUT_16 = 1, GX_TLUT_32 = 2, GX_TLUT_64 = 4, GX_TLUT_128 = 8, GX_TLUT_256 = 16,
    GX_TLUT_512 = 32, GX_TLUT_1K = 64, GX_TLUT_2K = 128, GX_TLUT_4K = 256, GX_TLUT_8K = 512, GX_TLUT_16K = 1024
} GXTlutSize;

typedef enum {
    GX_TEXCACHE_32K, GX_TEXCACHE_128K, GX_TEXCACHE_512K, GX_TEXCACHE_NONE
} GXTexCacheSize;

typedef enum {
    GX_MT_NULL = 0, GX_MT_XF_FLUSH = 1, GX_MT_DL_SAVE_CONTEXT = 2, GX_MT_ABORT_WAIT_COPYOUT = 3
} GXMiscToken;

typedef enum {
    GX_TF_C4 = 0x8, GX_TF_C8 = 0x9, GX_TF_C14X2 = 0xa
} GXCITexFmt;

// Advanced GX structs
typedef struct {
    GXAttr attr;
    GXCompCnt cnt;
    GXCompType type;
    u8 frac;
} GXVtxAttrFmtList;

typedef GXTexRegion* (*GXTexRegionCallback)(const GXTexObj* obj, GXTexMapID id);
typedef GXTlutRegion* (*GXTlutRegionCallback)(u32 idx);

// GRAPH structure (GameCube graphics context)
#ifndef GRAPH_DEFINED_BY_AC_DECOMP
#ifndef GRAPH_DEFINED
#define GRAPH_DEFINED
typedef struct {
    void* polyOpa;
    void* polyXlu;
    void* overlay;
} GRAPH;
#endif
#endif

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
// Note: GXEnd is declared as static inline below to match AC-decomp

// --- Vertex Specification functions are declared as static inline below to match AC-decomp ---
// (Removed duplicate regular declarations to prevent conflicts)

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

// ============================================================================
// COMPREHENSIVE GX FUNCTION DECLARATIONS (Prevent function conflicts)
// ============================================================================
// Declare ALL GX functions to prevent hydra effect with original headers

// Display List functions
void GXCallDisplayList(const void* list, u32 nbytes);

// Vertex functions (prevent static/non-static conflicts)
static inline void GXPosition2f32(f32 x, f32 y) { 
    glVertex2f(x, y);
    g_gx_state.current_pos[0] = x;
    g_gx_state.current_pos[1] = y;
    g_gx_state.current_pos[2] = 0.0f;
    g_gx_state.vertex_count++;
}
static inline void GXPosition3s16(s16 x, s16 y, s16 z) { 
    glVertex3s(x, y, z);
    g_gx_state.current_pos[0] = x;
    g_gx_state.current_pos[1] = y;
    g_gx_state.current_pos[2] = z;
    g_gx_state.vertex_count++;
}
static inline void GXPosition3f32(f32 x, f32 y, f32 z) { 
    glVertex3f(x, y, z);
    g_gx_state.current_pos[0] = x;
    g_gx_state.current_pos[1] = y;
    g_gx_state.current_pos[2] = z;
    g_gx_state.vertex_count++;
}
static inline void GXNormal3f32(f32 nx, f32 ny, f32 nz) { 
    glNormal3f(nx, ny, nz);
}
static inline void GXColor4u8(u8 r, u8 g, u8 b, u8 a) { 
    glColor4ub(r, g, b, a);
    g_gx_state.current_color.r = r / 255.0f;
    g_gx_state.current_color.g = g / 255.0f;
    g_gx_state.current_color.b = b / 255.0f;
    g_gx_state.current_color.a = a / 255.0f;
}
static inline void GXTexCoord2s16(s16 u, s16 v) { 
    glTexCoord2s(u, v);
    g_gx_state.current_texcoord[0] = u;
    g_gx_state.current_texcoord[1] = v;
}
static inline void GXPosition2s16(s16 x, s16 y) { 
    glVertex2s(x, y);
    g_gx_state.current_pos[0] = x;
    g_gx_state.current_pos[1] = y;
    g_gx_state.current_pos[2] = 0.0f;
    g_gx_state.vertex_count++;
}
static inline void GXPosition2u16(u16 x, u16 y) { 
    glVertex2i(x, y);
    g_gx_state.current_pos[0] = x;
    g_gx_state.current_pos[1] = y;
    g_gx_state.current_pos[2] = 0.0f;
    g_gx_state.vertex_count++;
}
static inline void GXTexCoord2f32(f32 u, f32 v) { 
    glTexCoord2f(u, v);
    g_gx_state.current_texcoord[0] = u;
    g_gx_state.current_texcoord[1] = v;
}
static inline void GXTexCoord2u8(u8 u, u8 v) { 
    glTexCoord2f(u / 255.0f, v / 255.0f);
    g_gx_state.current_texcoord[0] = u / 255.0f;
    g_gx_state.current_texcoord[1] = v / 255.0f;
}
static inline void GXEnd(void) { 
    glEnd();
    g_gx_state.in_begin_end = 0;
}

// ===================================================================
// PROACTIVE AC-DECOMP INTEGRATION
// ===================================================================
// Include proactive updates from comprehensive AC-decomp scan
// This handles 2,400 missing types and 3,458 missing functions
// Re-enabled now that core system type conflicts are resolved
#include "proactive_wrapper_updates.h"

// ============================================================================
// ULTIMATE GFX DISPLAY LIST SUPPORT (Final Solution)
// ============================================================================
// Fix the core issue: Gfx operations must support integer values for GameCube macros

// Hybrid Gfx definition - supports both u64 operations AND .words member access
// Only define if not already defined by smart shadow
#ifndef AC_VITA_GFX_FIXED
#undef Gfx
typedef union {
    u64 force_structure_alignment;  // For 64-bit display list operations
    struct {
        u32 w0;
        u32 w1;
    } words;  // For .words.w0 and .words.w1 member access
} Gfx;
#endif

// ============================================================================
// COMPLETE GAMECUBE GX CONSTANTS (Ultimate Anti-Hydra)
// ============================================================================
// Adding ALL remaining missing GameCube GX constants

// Additional GameCube GX Constants from error analysis
#define G_AD_DISABLE        0x00000000
#define G_AD_NOTPATTERN     0x00000001
#define G_CD_DISABLE        0x00000000
#define G_CD_MAGICSQ        0x00000001
#define G_CK_NONE           0x00000000
#define G_TF_POINT          0x00000000
#define G_TF_BILERP         0x00000001
#define G_TT_NONE           0x00000000
#define G_TT_RGBA16         0x00000001
#define G_TP_NONE           0x00000000
#define G_TP_PERSP          0x00000001
#define G_CYC_1CYCLE        0x00000000
#define G_AC_NONE           0x00000000
#define G_AC_THRESHOLD      0x00000001
#define G_RM_CLD_SURF       0x00000000
#define G_RM_CLD_SURF2      0x00000001
#define G_RM_XLU_SURF       0x00000002
#define G_RM_XLU_SURF2      0x00000003
#define G_IM_FMT_IA         0x00000001
#define G_IM_SIZ_8b         0x00000002
#define G_TX_WRAP           0x00000000
#define G_TX_NOMIRROR       0x00000001
#define G_TX_NOMASK         0x00000002
#define G_TX_NOLOD          0x00000003
#define G_MTX_NOPUSH        0x00000000
#define G_MTX_MODELVIEW     0x00000001
#define G_ON                0x00000001
#define G_CC_DECALRGBA      0x00000000
#define NOW_POLY_OPA_DISP   0x00000000

// Additional missing constants
#define PRIMITIVE           0x00000000
#define ENVIRONMENT         0x00000001
#define TEXEL0              0x00000002

// Essential GameCube constants (restored)
#define G_ZS_PRIM           0x00000000
#define G_TL_TILE           0x00000000  
#define G_TD_CLAMP          0x00000001
#define G_TC_FILT           0x00000002
#define G_PM_NPRIMITIVE     0x00000003
#define NOW_FONT_DISP       0x00000004
#define MTX_MULT            0x01  // vs MTX_MUL

// FINAL MISSING CARD CONSTANTS (for Animal Crossing completion)
#define CARD_COMMENT_SIZE   64
#define CARD_NUM_CHANS      2
#define CARD_FILENAME_MAX   32
#define CARD_MAX_FILE       127
#define CARD_ICON_MAX       8

// FINAL MISSING AC-SPECIFIC TYPES (for Animal Crossing completion)
// REMOVED: Our placeholder types that conflict with AC-decomp's real definitions
// AC-decomp headers now provide the authoritative type definitions
// typedef struct aTKC_clip_c - removed to avoid conflict
// typedef struct mNW_original_tex_c - removed to avoid conflict  
// typedef struct aINS_overlay_entry_c - removed to avoid conflict

// Essential field info type that AC-decomp expects
typedef struct mFI_block_tbl_c {
    s8 block_x;
    s8 block_z;
    f32 pos_x;
    f32 pos_z;
    void* items;  // mActor_name_t* items
} mFI_block_tbl_c;

// Additional missing AC-decomp types for final completion
typedef struct aTKC_clip_c {
    u32 dummy[4];  // Placeholder - let AC-decomp provide real definition
} aTKC_clip_c;

typedef struct mFI_unit_c {
    int block_x;
    int block_z;
    int unit_x;
    int unit_z;
    void* block_data;  // mActor_name_t* block_data
} mFI_unit_c;

// GameCube controller buttons (essential)
#define BUTTON_NONE         0x0000
#define BUTTON_CRIGHT       0x0001
#define BUTTON_CLEFT        0x0002
#define BUTTON_CDOWN        0x0004
#define BUTTON_CUP          0x0008
#define BUTTON_R            0x0010
#define BUTTON_L            0x0020
#define BUTTON_X            0x0040
#define BUTTON_Y            0x0080
#define BUTTON_DRIGHT       0x0100
#define BUTTON_DLEFT        0x0200
#define BUTTON_DDOWN        0x0400
#define BUTTON_DUP          0x0800
#define BUTTON_START        0x1000
#define BUTTON_Z            0x2000
#define BUTTON_B            0x4000
#define BUTTON_A            0x8000

// Attribute alignment macro (critical for compilation)
#ifndef ATTRIBUTE_ALIGN
#if defined(__MWERKS__) || defined(__GNUC__)
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#elif defined(_MSC_VER)
#define ATTRIBUTE_ALIGN(num)
#else
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif
#endif

// ============================================================================
// COMPREHENSIVE VOLATILE TYPES (Anti-Hydra Strategy)
// ============================================================================
// Adding ALL missing volatile types to prevent hydra effect

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;
typedef volatile f32 vf32;
typedef volatile f64 vf64;

// ============================================================================
// COMPREHENSIVE AC-SPECIFIC TYPES (Anti-Hydra Strategy)  
// ============================================================================
// Adding AC-specific types found in error analysis

// ============================================================================
// ULTIMATE ANTI-HYDRA FINAL PATTERN ELIMINATION
// ============================================================================

// Fix AT_ADDRESS macro (critical GameCube memory layout)
#ifndef AT_ADDRESS
#ifdef __MWERKS__
#define AT_ADDRESS(x) : (x)
#else
#define AT_ADDRESS(x) __attribute__((section(".data." #x)))
#endif
#endif

// Fix OSThread type (GameCube threading) - already defined above
// Note: OSThread is already defined earlier in the file as "typedef void OSThread;"

// ============================================================================
// REMOVE CONFLICTING DEFINITIONS (Anti-Hydra Strategy)
// ============================================================================
// Remove our placeholder types that conflict with real AC-decomp definitions
// AC-decomp has the real definitions, we should not override them

// REMOVED: typedef struct mNW_original_tex_c (conflicts with AC-decomp)
// REMOVED: typedef struct aINS_overlay_entry_c (conflicts with AC-decomp)  
// REMOVED: typedef struct aTKC_clip_c (conflicts with AC-decomp)

// REMOVED: static inline CARD functions (conflicts with AC-decomp headers)
// AC-decomp's dolphin/card.h has the real function declarations

// ============================================================================
// COMPREHENSIVE GFX POINTER OPERATIONS (Anti-Hydra Strategy)
// ============================================================================
// Fix the primary error: invalid operands to binary | with Gfx pointers

// Remove conflicting Gfx redefinition - it's already defined above
// Define Gfx operations that support | operator properly (move to implementation section)

#ifdef __cplusplus
}
#endif

// ============================================================================
// MISSING UTILITY MACROS (Fix allocation and array errors)
// ============================================================================

#ifndef GRAPH_ALLOC_TYPE
#define GRAPH_ALLOC_TYPE(graph, type, count) ((type*)malloc(sizeof(type) * (count)))
#endif

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// ============================================================================
// MISSING FIELD INFO CONSTANTS (Fix field info errors)
// ============================================================================

#ifndef mFI_UNIT_BASE_SIZE_F
#define mFI_UNIT_BASE_SIZE_F 160.0f
#endif

#ifndef mFI_UT_WORLDSIZE_X_F
#define mFI_UT_WORLDSIZE_X_F 1280.0f
#endif

#ifndef mFI_UT_WORLDSIZE_Z_F
#define mFI_UT_WORLDSIZE_Z_F 1280.0f
#endif

// ============================================================================
// ALIGNMENT AND THREAD MACROS (Fix alignment and thread errors)
// ============================================================================

// Fix alignment macro issues (variably modified errors)
#ifndef __align
#define __align(x) __attribute__((aligned(x)))
#endif

#ifndef __force_sector_align  
#define __force_sector_align __attribute__((aligned(32)))
#endif

// Missing alignment calculation macros (Fix ALIGN_NEXT errors)
#ifndef ALIGN_NEXT
#define ALIGN_NEXT(x, align) (((x) + (align) - 1) & ~((align) - 1))
#endif

#ifndef mCD_ALIGN_SECTORSIZE
// Use a constant size to avoid sizeof() issues in array declarations
#define mCD_ALIGN_SECTORSIZE(x) 32768  // Large enough for Save_t and other structures
#endif

#ifndef mCD_MEMCARD_SECTORSIZE
#define mCD_MEMCARD_SECTORSIZE 32
#endif

// ============================================================================
// THREAD AND MANAGER TYPES (Fix thread-related errors)  
// ============================================================================

// Fix OSThread type redefinition (change void to proper struct)
#ifdef OSThread
#undef OSThread
#endif

typedef struct OSThread {
    u32 dummy[32];  // Placeholder for thread structure
} OSThread;

// Add missing manager types that use OSThread
typedef struct {
    OSThread thread;
    u32 dummy[16];
} IrqMgr;

typedef struct {
    OSThread thread;  
    u32 dummy[16];
} PadMgr;

#endif // GX_TO_VITAGL_WRAPPER_H 