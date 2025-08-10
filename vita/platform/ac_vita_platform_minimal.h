/**
 * AC Vita Platform - Minimal Compatibility Layer
 * 
 * This header provides ONLY essential function implementations
 * and lets AC-Decomp define all types, constants, and structures.
 */

#ifndef AC_VITA_PLATFORM_MINIMAL_H
#define AC_VITA_PLATFORM_MINIMAL_H

#include <stddef.h>  // For size_t
#include <stdint.h>  // For standard types

// Block MSL headers completely
#define _MSL_ANSI_FILES_H
#define _MSL_W_MATH_H  
#define _MSL_COMMON_FLOAT_H

// Prevent AC-Decomp from redefining size_t (we use system size_t)
#define _SIZE_T_DEF

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ESSENTIAL TYPES (Only what AC-Decomp absolutely needs)
// =============================================================================

// Display list command structure (GameCube/N64 style)
#ifndef GFX_DEFINED
#define GFX_DEFINED
typedef struct {
    union {
        struct {
            unsigned int w0;  // First word of display list command
            unsigned int w1;  // Second word of display list command  
        };
        struct {
            unsigned int w0;
            unsigned int w1;
        } words;  // AC-Decomp expects gfx->words.w0 and gfx->words.w1
    };
} Gfx;
#endif

// Matrix type (GameCube 3x4 matrix)
#ifndef MTX_DEFINED
#define MTX_DEFINED
typedef float Mtx[3][4];
#endif

// Vertex structure (GameCube/N64 style)
#ifndef VTX_DEFINED
#define VTX_DEFINED
typedef struct {
    short pos[3];      // Position (x, y, z)
    unsigned short flag; // Flags
    short tc[2];       // Texture coordinates (s, t)
    unsigned char cn[4]; // Color and normal data
} Vtx;
#endif

// Additional graphics types needed by libultra
#ifndef LOOKAT_DEFINED
#define LOOKAT_DEFINED
typedef struct {
    float l[9];  // Look-at matrix data
} LookAt;
#endif

#ifndef HILITE_DEFINED
#define HILITE_DEFINED  
typedef struct {
    float h[9];  // Hilite data
} Hilite;
#endif

// Display list combiner constants (common values)
#ifndef TEXEL0
#define TEXEL0          0x01
#define ENVIRONMENT     0x03
#define COMBINED        0x00

// Texture constants
#define G_TX_NOMIRROR   0x00
#define G_TX_WRAP       0x01
#define G_TX_NOMASK     0x00
#define G_TX_NOLOD      0x00
#define G_TX_RENDERTILE 0x07

// Additional graphics constants that AC-Decomp needs
#define G_MWO_SEGMENT_D 0x08
#define G_SPECIAL_1 0x01
#define G_SPECIAL_NONE 0x00

// Graphics pipeline constants (TEV system)
#define PRIMITIVE 0
#define ENVIRONMENT 1
#define TEXEL0 2
#define TEXEL1 3
#define TEV_PRIMITIVE 0

// Geometry mode constants
#define G_CULL_BACK 0x00001000

// Math constants that AC-Decomp expects
#ifndef F_PI
#define F_PI 3.14159265358979323846f
#endif

// Missing graphics structures
typedef struct {
    unsigned int format;
    void* img_ptr;
    unsigned short width;
} Gsetimg;

typedef struct {
    unsigned char col[3];
    signed char dir[3];
} Light_t;

// Lighting and viewport types
typedef struct {
    unsigned char col[3];
    char pad;
} Ambient;

// Note: Hilite union is already properly defined by AC-Decomp in PR/gbi.h

typedef struct {
    struct {
        float vscale[4];
        float vtrans[4];
    } vp;
} Vp;

#endif

// =============================================================================
// PLATFORM INITIALIZATION (Essential functions only)  
// =============================================================================

// Forward declaration of GRAPH (to avoid conflicts)
typedef struct graph_s GRAPH;

int ac_vita_platform_init(void);
void ac_vita_platform_cleanup(void);

// Graph management for AC-Decomp
void ac_vita_open_disp(GRAPH* graph);
void ac_vita_close_disp(GRAPH* graph); 
void* ac_vita_graph_alloc(GRAPH* graph, size_t size);

// AC-Decomp specific render state functions (let AC-Decomp define the GRAPH type)
void _texture_z_light_fog_prim(GRAPH* graph);
void _texture_z_light_fog_prim_xlu(GRAPH* graph);
void _texture_z_light_fog_prim_bg(GRAPH* graph);
void _texture_z_light_fog_prim_shadow(GRAPH* graph);
void _texture_z_light_fog_prim_light(GRAPH* graph);
void _texture_z_light_fog_prim_npc(GRAPH* graph);

// Asset bridge functions
unsigned char* ac_load_texture_by_name(const char* name);
int ac_asset_exists(const char* name);

// =============================================================================
// DISPLAY LIST FUNCTION DECLARATIONS (VitaGL Wrappers)
// =============================================================================

// Display list macros (AC-Decomp expects compile-time constants for array initialization)
// These must be macros, not functions, to work in static initializers
#define gsSPLoadGeometryMode(mode) {(unsigned int)(0xD7000000 | ((mode) & 0xFFFFFF)), 0}
#define gsDPSetOtherMode(mode1, mode2) {(unsigned int)(0xEF000000 | ((mode1) & 0xFFFF)), (unsigned int)((mode2) & 0xFFFFFFFF)}
#define gsEndDisplayList() {0xDF000000, 0}

// Core display list functions that AC-Decomp uses (fixed signatures to match AC-Decomp calling convention)
void gDPSetCombineLERP(Gfx* gfx, 
    unsigned int a0, unsigned int b0, unsigned int c0, unsigned int d0,
    unsigned int Aa0, unsigned int Ab0, unsigned int Ac0, unsigned int Ad0,
    unsigned int a1, unsigned int b1, unsigned int c1, unsigned int d1,
    unsigned int Aa1, unsigned int Ab1, unsigned int Ac1, unsigned int Ad1);

void gDPSetEnvColor(Gfx* gfx, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void gDPPipeSync(Gfx* gfx);
void gDPSetOtherMode(Gfx* gfx, unsigned int mode1, unsigned int mode2);
void gDPFillRectangle(Gfx* gfx, int ulx, int uly, int lrx, int lry);

void gSPTextureRectangle(Gfx* gfx, int ulx, int uly, int lrx, int lry, 
                        int tile, int uls, int ult, int dsdx, int dtdy);

void gDPLoadTextureTile(Gfx* gfx, void* timg, unsigned int fmt, unsigned int siz,
                       unsigned int width, unsigned int height, 
                       unsigned int uls, unsigned int ult, unsigned int lrs, unsigned int lrt,
                       unsigned int pal, unsigned int cms, unsigned int cmt,
                       unsigned int masks, unsigned int maskt,
                       unsigned int shifts, unsigned int shiftt);

void gSPLoadUcode(Gfx* gfx, void* ucode_start, void* ucode_data);
void gDPLoadTLUT(Gfx* gfx, unsigned int count, unsigned int tmem, void* tlut);
void gSPBgRectCopy(Gfx* gfx, void* bg);
void gSPObjRenderMode(Gfx* gfx, unsigned int mode);
void gSPBgRect1Cyc(Gfx* gfx, void* bg);

void gDPSetColorImage(Gfx* gfx, unsigned int fmt, unsigned int siz, 
                     unsigned int width, void* img);
void gDPSetScissor(Gfx* gfx, unsigned int mode, unsigned int ulx, unsigned int uly,
                  unsigned int lrx, unsigned int lry);
void gDPSetBlendColor(Gfx* gfx, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void gDPSetPrimDepth(Gfx* gfx, int z, int dz);

// Additional functions that AC-Decomp uses
void gSPLookAt(Gfx* gfx, void* lookat);
void gDPSetTileSize(Gfx* gfx, unsigned int tile, unsigned int uls, unsigned int ult, unsigned int lrs, unsigned int lrt);
void gDma0p(Gfx* gfx, unsigned int cmd, void* ptr, unsigned int len);
void gImmp1(Gfx* gfx, unsigned int cmd, unsigned int data);

#ifdef __cplusplus
}
#endif

#endif // AC_VITA_PLATFORM_MINIMAL_H 
// ============================================================================= 
// COMPREHENSIVE GRAPHICS CONSTANTS (Pipeline Fix for 3,206 files)
// =============================================================================

// Missing SHADE constants
#ifndef SHADE
#define SHADE 0
#endif

// Missing G_MWO_SEGMENT constants  
#define G_MWO_SEGMENT_8 0x08
#define G_MWO_SEGMENT_A 0x0A
#define G_MWO_SEGMENT_C 0x0C
#define G_MWO_SEGMENT_E 0x0E


