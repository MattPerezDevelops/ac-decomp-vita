/*
 * Animal Crossing Vita - Demo Main
 * ================================
 * Simple demo using our smart shadow system and GX wrapper
 */

#include "platform/combined_wrapper.h"
#include <vitaGL.h>
#include <psp2/ctrl.h>

int main() {
    // Initialize VitaGL
    vglInitExtended(0x1000000, 960, 544, 0x6000000, SCE_GXM_MULTISAMPLE_4X);
    
    // Test our type system
    s32 test_s32 = -42;
    u32 test_u32 = 42;
    f32 test_f32 = 3.14f;
    
    // Test our Gfx union
    Gfx test_gfx;
    test_gfx.words.w0 = 0x12345678;
    test_gfx.words.w1 = 0x87654321;
    
    // Simple render loop
    for (int i = 0; i < 60; i++) {
        vglStartRendering();
        
        // Clear screen
        glClearColor(0.2f, 0.6f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Test our wrapper functions work
        glColor3f(1.0f, 1.0f, 1.0f);
        
        vglStopRenderingInit();
        vglStopRenderingTerm();
        glFinish();
    }
    
    vglEnd();
    return 0;
}
