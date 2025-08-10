/**
 * Animal Crossing Vita - Minimal Implementation
 * VitaGL wrappers for GameCube graphics functions
 */

#include "ac_vita_platform_minimal.h"
#include <vitaGL.h>
#include <stdlib.h>

// =============================================================================
// VITAGL WRAPPER IMPLEMENTATIONS
// =============================================================================

int ac_vita_platform_init(void) {
    vglInit(0x800000);
    vglUseTripleBuffering(GL_TRUE);
    return 1;
}

void ac_vita_platform_cleanup(void) {
    vglEnd();
}

// GameCube display list function implementations (VitaGL wrappers)
void gDPSetCombineLERP(Gfx* gfx, 
    unsigned int a0, unsigned int b0, unsigned int c0, unsigned int d0,
    unsigned int Aa0, unsigned int Ab0, unsigned int Ac0, unsigned int Ad0,
    unsigned int a1, unsigned int b1, unsigned int c1, unsigned int d1,
    unsigned int Aa1, unsigned int Ab1, unsigned int Ac1, unsigned int Ad1) {
    // Set color combiner - simplified for VitaGL
}

void gDPSetEnvColor(Gfx* gfx, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    glColor4ub(r, g, b, a);
}

void gDPPipeSync(Gfx* gfx) {
    glFlush();
}

void gDPFillRectangle(Gfx* gfx, int ulx, int uly, int lrx, int lry) {
    glBegin(GL_QUADS);
    glVertex2f(ulx, uly);
    glVertex2f(lrx, uly);
    glVertex2f(lrx, lry);
    glVertex2f(ulx, lry);
    glEnd();
}

void gSPTextureRectangle(Gfx* gfx, int ulx, int uly, int lrx, int lry,
                        int tile, int uls, int ult, int dsdx, int dtdy) {
    glBegin(GL_QUADS);
    glTexCoord2f(uls/1024.0f, ult/1024.0f); glVertex2f(ulx, uly);
    glTexCoord2f((uls+dsdx)/1024.0f, ult/1024.0f); glVertex2f(lrx, uly);
    glTexCoord2f((uls+dsdx)/1024.0f, (ult+dtdy)/1024.0f); glVertex2f(lrx, lry);
    glTexCoord2f(uls/1024.0f, (ult+dtdy)/1024.0f); glVertex2f(ulx, lry);
    glEnd();
}

// Stub implementations for other functions
void gSPLookAt(Gfx* gfx, void* lookat) {
    // Set lookat parameters - no-op for VitaGL
}

void gDPSetTileSize(Gfx* gfx, unsigned int tile, unsigned int uls, unsigned int ult, unsigned int lrs, unsigned int lrt) {
    // Set texture tile size parameters - no-op for VitaGL
}

void gDma0p(Gfx* gfx, unsigned int cmd, void* ptr, unsigned int len) {
    // DMA operation - no-op for VitaGL
}

void gImmp1(Gfx* gfx, unsigned int cmd, unsigned int data) {
    // Immediate mode operation - no-op for VitaGL  
}

// Additional stubs
void* gfxalloc(size_t size) {
    return malloc(size);
}

void* ucode_GetPolyTextStart(void) { return NULL; }
void* ucode_GetPolyDataStart(void) { return NULL; }
void* ucode_GetSpriteTextStart(void) { return NULL; }
void* ucode_GetSpriteDataStart(void) { return NULL; }
