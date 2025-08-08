#ifndef AC_VITA_PLATFORM_H
#define AC_VITA_PLATFORM_H

// System includes first
#include <psp2/types.h>
#include <psp2/ctrl.h>
#include <vitaGL.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ===================================================================
// ESSENTIAL GAMECUBE TYPES (let AC-Decomp define basic types)
// ===================================================================

// Graphics types (only define what AC-Decomp doesn't have)
#ifndef GFX_DEFINED
#define GFX_DEFINED
typedef struct { unsigned int cmd[2]; } Gfx;
#endif

#ifndef GSETIMG_DEFINED  
#define GSETIMG_DEFINED
typedef struct { unsigned int cmd; void* data; } Gsetimg;
#endif

#ifndef LIGHT_T_DEFINED
#define LIGHT_T_DEFINED  
typedef struct { unsigned char r, g, b; signed char x, y, z; } Light_t;
#endif

// GameCube controller type (use basic types)
typedef struct { 
    unsigned short buttons; 
    signed char stick_x, stick_y; 
    signed char substick_x, substick_y; 
    unsigned char trigger_left, trigger_right; 
    unsigned char analog_a, analog_b; 
    signed char err; 
} PadStatus;

// GameCube graphics types
typedef unsigned int GXPrimitive;
typedef struct { 
    void* data; 
    unsigned short width; 
    unsigned short height; 
    unsigned int format; 
    unsigned int wrap_s, wrap_t; 
    unsigned int mipmap; 
} GXTexObj;

// Matrix types (use basic types)
#ifndef MTX_DEFINED
#define MTX_DEFINED
typedef signed short Mtx[4][4];
#endif

#ifndef MTXF_DEFINED
#define MTXF_DEFINED
typedef float MtxF[4][4];
#endif

// Vertex type (use basic types)
#ifndef VTX_DEFINED
#define VTX_DEFINED
typedef struct { 
    float pos[3];    // Position
    float norm[3];   // Normal
    float tc[2];     // Texture coordinates
    unsigned char rgba[4];    // Color
} Vtx;
#endif

// GameCube constants
#define GX_TRIANGLES 0x90
#define GX_QUADS 0x80
#define GX_TRIANGLE_STRIP 0x98
#define GX_TRIANGLE_FAN 0x92

// Texture constants
#define TEXEL0 0
#define ENVIRONMENT 1
#define COMBINED 2
#define G_TX_NOMIRROR 0x00
#define G_TX_WRAP 0x02
#define G_TX_NOMASK 0x00
#define G_TX_NOLOD 0x00
#define G_TX_RENDERTILE 0x07
#define G_TEXTURE_IMAGE_FRAC 2
#define G_IM_FMT_I 4
#define G_IM_SIZ_8b 1

// Platform functions
int ac_vita_platform_init(void);
void ac_vita_platform_cleanup(void);
void ac_vita_frame_begin(void);
void ac_vita_frame_end(void);

// Controller functions
void ac_vita_read_controller(PadStatus* pad);

// Graphics functions (GameCube GX API → VitaGL) - use basic type names
void GXBegin(GXPrimitive primitive, unsigned int vtxfmt, unsigned short nverts);
void GXColor4u8(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void GXPosition2f32(float x, float y);
void GXEnd(void);
void GXNormal3f32(float x, float y, float z);
void GXInitTexObj(GXTexObj* obj, void* data, unsigned short width, unsigned short height, unsigned int format, unsigned int wrap_s, unsigned int wrap_t, unsigned int mipmap);
void GXLoadTexObj(GXTexObj* obj, unsigned int mapid);

// Display list functions - use basic type names
void gDPPipeSync(Gfx* gfx);
void gDPFullSync(Gfx* gfx);
void gDPSetPrimColor(Gfx* gfx, unsigned int minlevel, unsigned int maxlevel, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void gDPSetEnvColor(Gfx* gfx, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void gDPSetRenderMode(Gfx* gfx, unsigned int mode1, unsigned int mode2);
void gDPLoadTextureTile(Gfx* gfx, void* timg, unsigned int fmt, unsigned int siz, unsigned int width, unsigned int height, unsigned int uls, unsigned int ult, unsigned int lrs, unsigned int lrt, unsigned int pal, unsigned int cms, unsigned int cmt, unsigned int masks, unsigned int maskt, unsigned int shifts, unsigned int shiftt);
void gSPMatrix(Gfx* gfx, Mtx* m, unsigned char param);
void gSPVertex(Gfx* gfx, void* v, unsigned int n, unsigned int v0);
void gSP2Triangles(Gfx* gfx, unsigned int v0, unsigned int v1, unsigned int v2, unsigned int flag0, unsigned int v3, unsigned int v4, unsigned int v5, unsigned int flag1);
void gSPDisplayList(Gfx* gfx, Gfx* dl);
void gSPBranchList(Gfx* gfx, Gfx* dl);
void gSPEndDisplayList(Gfx* gfx);

// Additional graphics functions
void gSPLoadUcode(Gfx* gfx, void* ucode, void* ucode_data);
void gDPLoadTLUT(Gfx* gfx, unsigned int tile, unsigned int count, void* tlut);
void gDPSetOtherMode(Gfx* gfx, unsigned int mode0, unsigned int mode1);
void gDPSetCombineLERP(Gfx* gfx, unsigned int a0, unsigned int b0, unsigned int c0, unsigned int d0, unsigned int a1, unsigned int b1, unsigned int c1, unsigned int d1, unsigned int a2, unsigned int b2, unsigned int c2, unsigned int d2, unsigned int a3, unsigned int b3, unsigned int c3, unsigned int d3);
void gDPSetBlendColor(Gfx* gfx, unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void gDPSetPrimDepth(Gfx* gfx, unsigned short z, unsigned short dz);
void gSPTextureRectangle(Gfx* gfx, unsigned int xl, unsigned int yl, unsigned int xh, unsigned int yh, unsigned int tile, unsigned int s, unsigned int t, unsigned int dsdx, unsigned int dtdy);
void gDma0p(Gfx* gfx, unsigned int mode, void* ptr, unsigned int param);
void gImmp1(Gfx* gfx, unsigned int cmd, unsigned int data);
void gDPSetColorImage(Gfx* gfx, unsigned int fmt, unsigned int siz, unsigned int width, void* img);
void gDPSetScissor(Gfx* gfx, unsigned int mode, unsigned int ulx, unsigned int uly, unsigned int lrx, unsigned int lry);
void gDPFillRectangle(Gfx* gfx, unsigned int ulx, unsigned int uly, unsigned int lrx, unsigned int lry);

// Memory/utility functions
void bzero_blocked_by_shadow(void* ptr, size_t size);

#endif // AC_VITA_PLATFORM_H
