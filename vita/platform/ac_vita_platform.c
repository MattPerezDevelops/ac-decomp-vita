#include "ac_vita_platform.h"

// Global variables
Gfx* gfx_ptr[3] = {NULL, NULL, NULL};
static int platform_initialized = 0;

// Platform initialization
int ac_vita_platform_init(void) {
    if (platform_initialized) return 1;
    
    // Initialize VitaGL
    vglInitExtended(0x1000000, 960, 544, 0x6000000, SCE_GXM_MULTISAMPLE_4X);
    glViewport(0, 0, 960, 544);
    glEnable(GL_DEPTH_TEST);
    
    platform_initialized = 1;
    return 1;
}

void ac_vita_platform_cleanup(void) {
    vglEnd();
    platform_initialized = 0;
}

void ac_vita_frame_begin(void) {
    vglStartRendering();
}

void ac_vita_frame_end(void) {
    vglStopRenderingInit();
    vglStopRenderingTerm();
}

// Controller functions
void ac_vita_read_controller(PadStatus* pad) {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    
    pad->buttons = 0;
    if (ctrl.buttons & SCE_CTRL_CROSS) pad->buttons |= 1;
    if (ctrl.buttons & SCE_CTRL_CIRCLE) pad->buttons |= 2;
    if (ctrl.buttons & SCE_CTRL_SQUARE) pad->buttons |= 4;
    if (ctrl.buttons & SCE_CTRL_TRIANGLE) pad->buttons |= 8;
    
    pad->stick_x = (ctrl.lx - 128) / 128.0f * 127;
    pad->stick_y = (ctrl.ly - 128) / 128.0f * 127;
}

// Graphics functions
void GXBegin(GXPrimitive primitive, unsigned int vtxfmt, unsigned short nverts) {
    (void)vtxfmt; (void)nverts;
    switch (primitive) {
        case GX_TRIANGLES:
            glBegin(GL_TRIANGLES);
            break;
        case GX_QUADS:
            glBegin(GL_QUADS);
            break;
        case GX_TRIANGLE_STRIP:
            glBegin(GL_TRIANGLE_STRIP);
            break;
        case GX_TRIANGLE_FAN:
            glBegin(GL_TRIANGLE_FAN);
            break;
        default:
            glBegin(GL_TRIANGLES);
            break;
    }
}

void GXColor4u8(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    glColor4ub(r, g, b, a);
}

void GXPosition2f32(float x, float y) {
    glVertex2f(x, y);
}

void GXPosition3f32(float x, float y, float z) {
    glVertex3f(x, y, z);
}

void GXEnd(void) {
    glEnd();
}

void GXNormal3f32(float x, float y, float z) {
    // glNormal3f not available in VitaGL
    (void)x; (void)y; (void)z;
}

void GXInitTexObj(GXTexObj* obj, void* data, unsigned short width, unsigned short height, unsigned int format, unsigned int wrap_s, unsigned int wrap_t, unsigned int mipmap) {
    obj->data = data;
    obj->width = width;
    obj->height = height;
    obj->format = format;
    obj->wrap_s = wrap_s;
    obj->wrap_t = wrap_t;
    obj->mipmap = mipmap;
}

void GXLoadTexObj(GXTexObj* obj, unsigned int mapid) {
    (void)mapid;
    if (obj && obj->data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, obj->width, obj->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, obj->data);
    }
}

// Display list functions - all implemented as placeholders
void gDPPipeSync(Gfx* gfx) { (void)gfx; glFinish(); }
void gDPFullSync(Gfx* gfx) { (void)gfx; glFinish(); }
void gDPSetPrimColor(Gfx* gfx, unsigned int minlevel, unsigned int maxlevel, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)gfx; (void)minlevel; (void)maxlevel; (void)r; (void)g; (void)b; (void)a; }
void gDPSetEnvColor(Gfx* gfx, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { (void)gfx; (void)r; (void)g; (void)b; (void)a; }
void gDPSetRenderMode(Gfx* gfx, unsigned int mode1, unsigned int mode2) { (void)gfx; (void)mode1; (void)mode2; }
void gDPLoadTextureTile(Gfx* gfx, void* timg, unsigned int fmt, unsigned int siz, unsigned int width, unsigned int height, unsigned int uls, unsigned int ult, unsigned int lrs, unsigned int lrt, unsigned int pal, unsigned int cms, unsigned int cmt, unsigned int masks, unsigned int maskt, unsigned int shifts, unsigned int shiftt) {
    (void)gfx; (void)timg; (void)fmt; (void)siz; (void)width; (void)height; (void)uls; (void)ult; (void)lrs; (void)lrt; (void)pal; (void)cms; (void)cmt; (void)masks; (void)maskt; (void)shifts; (void)shiftt;
}
void gSPMatrix(Gfx* gfx, Mtx* m, unsigned char param) { (void)gfx; (void)m; (void)param; }
void gSPVertex(Gfx* gfx, void* v, unsigned int n, unsigned int v0) { (void)gfx; (void)v; (void)n; (void)v0; }
void gSP2Triangles(Gfx* gfx, unsigned int v0, unsigned int v1, unsigned int v2, unsigned int flag0, unsigned int v3, unsigned int v4, unsigned int v5, unsigned int flag1) {
    (void)gfx; (void)v0; (void)v1; (void)v2; (void)flag0; (void)v3; (void)v4; (void)v5; (void)flag1;
}
void gSPDisplayList(Gfx* gfx, Gfx* dl) { (void)gfx; (void)dl; }
void gSPBranchList(Gfx* gfx, Gfx* dl) { (void)gfx; (void)dl; }
void gSPEndDisplayList(Gfx* gfx) { (void)gfx; }

// Additional graphics functions
void gSPLoadUcode(Gfx* gfx, void* ucode, void* ucode_data) { (void)gfx; (void)ucode; (void)ucode_data; }
void gDPLoadTLUT(Gfx* gfx, unsigned int tile, unsigned int count, void* tlut) { (void)gfx; (void)tile; (void)count; (void)tlut; }
void gDPSetOtherMode(Gfx* gfx, unsigned int mode0, unsigned int mode1) { (void)gfx; (void)mode0; (void)mode1; }
void gDPSetCombineLERP(Gfx* gfx, unsigned int a0, unsigned int b0, unsigned int c0, unsigned int d0, unsigned int a1, unsigned int b1, unsigned int c1, unsigned int d1, unsigned int a2, unsigned int b2, unsigned int c2, unsigned int d2, unsigned int a3, unsigned int b3, unsigned int c3, unsigned int d3) { (void)gfx; (void)a0; (void)b0; (void)c0; (void)d0; (void)a1; (void)b1; (void)c1; (void)d1; (void)a2; (void)b2; (void)c2; (void)d2; (void)a3; (void)b3; (void)c3; (void)d3; }
void gDPSetBlendColor(Gfx* gfx, unsigned int r, unsigned int g, unsigned int b, unsigned int a) { (void)gfx; (void)r; (void)g; (void)b; (void)a; }
void gDPSetPrimDepth(Gfx* gfx, unsigned short z, unsigned short dz) { (void)gfx; (void)z; (void)dz; }
void gSPTextureRectangle(Gfx* gfx, unsigned int xl, unsigned int yl, unsigned int xh, unsigned int yh, unsigned int tile, unsigned int s, unsigned int t, unsigned int dsdx, unsigned int dtdy) { (void)gfx; (void)xl; (void)yl; (void)xh; (void)yh; (void)tile; (void)s; (void)t; (void)dsdx; (void)dtdy; }
void gDma0p(Gfx* gfx, unsigned int mode, void* ptr, unsigned int param) { (void)gfx; (void)mode; (void)ptr; (void)param; }
void gImmp1(Gfx* gfx, unsigned int cmd, unsigned int data) { (void)gfx; (void)cmd; (void)data; }
void gDPSetColorImage(Gfx* gfx, unsigned int fmt, unsigned int siz, unsigned int width, void* img) { (void)gfx; (void)fmt; (void)siz; (void)width; (void)img; }
void gDPSetScissor(Gfx* gfx, unsigned int mode, unsigned int ulx, unsigned int uly, unsigned int lrx, unsigned int lry) { (void)gfx; (void)mode; (void)ulx; (void)uly; (void)lrx; (void)lry; }
void gDPFillRectangle(Gfx* gfx, unsigned int ulx, unsigned int uly, unsigned int lrx, unsigned int lry) { (void)gfx; (void)ulx; (void)uly; (void)lrx; (void)lry; }

// Memory/utility functions
void bzero_blocked_by_shadow(void* ptr, size_t size) {
    memset(ptr, 0, size);
}

} 