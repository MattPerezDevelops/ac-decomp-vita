/**
 * AC-Decomp Vita Platform Wrapper Implementation
 * Automatic GameCube SDK -> VitaGL translation layer
 */

#include "ac_vita_platform.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool g_platform_initialized = false;
static bool g_frame_active = false;
static GLuint g_current_texture = 0;
static GXPrimitive g_current_primitive = GX_TRIANGLES;
static u16 g_vertex_count = 0;

// Performance tracking
static struct {
    OSTime frame_start_time;
    u32 frame_count;
    u32 draw_calls;
    u32 assets_loaded;
    u32 memory_used;
} g_perf_stats;

// Input state
static PADStatus g_pad_status = {0};
static SceCtrlData g_vita_ctrl = {0};

// Audio state
static int g_audio_port = -1;

// =============================================================================
// PLATFORM INITIALIZATION
// =============================================================================

int AC_Vita_Platform_Init(void) {
    if (g_platform_initialized) {
        return 1; // Already initialized
    }
    
    printf("🚀 AC-Decomp Vita Platform Initializing...\n");
    
    // Set CPU and GPU clocks for performance
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);  
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    
    // Initialize VitaGL
    printf("📺 Initializing VitaGL...\n");
    vglInitExtended(0x1000000, 960, 544, 0x6000000, SCE_GXM_MULTISAMPLE_4X);
    
    // Set up OpenGL state for Animal Crossing rendering
    glViewport(0, 0, 960, 544);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Initialize asset bridge
    printf("🌉 Initializing Asset Bridge...\n");
    if (!ac_bridge_init()) {
        printf("❌ Failed to initialize asset bridge\n");
        return 0;
    }
    
    // Initialize audio
    printf("🔊 Initializing Audio...\n");
    g_audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, 1024, 48000, SCE_AUDIO_OUT_MODE_STEREO);
    if (g_audio_port < 0) {
        printf("⚠️ Audio initialization failed, continuing without audio\n");
    }
    
    // Initialize performance tracking
    memset(&g_perf_stats, 0, sizeof(g_perf_stats));
    g_perf_stats.frame_start_time = OSGetTime();
    
    g_platform_initialized = true;
    printf("✅ AC-Decomp Vita Platform Ready!\n");
    return 1;
}

void AC_Vita_Platform_Cleanup(void) {
    if (!g_platform_initialized) return;
    
    printf("🛑 AC-Decomp Vita Platform Cleanup...\n");
    
    // Cleanup audio
    if (g_audio_port >= 0) {
        sceAudioOutReleasePort(g_audio_port);
        g_audio_port = -1;
    }
    
    // Cleanup asset bridge
    ac_bridge_cleanup();
    
    // Cleanup VitaGL
    vglEnd();
    
    g_platform_initialized = false;
    printf("✅ Cleanup complete\n");
}

// =============================================================================
// FRAME MANAGEMENT
// =============================================================================

void AC_Vita_Frame_Begin(void) {
    if (!g_platform_initialized) return;
    
    g_frame_active = true;
    g_perf_stats.frame_start_time = OSGetTime();
    g_perf_stats.draw_calls = 0;
    
    // Start VitaGL frame
    vglStartRendering();
    
    // Clear the screen (typical AC background color)
    glClearColor(0.53f, 0.81f, 0.94f, 1.0f); // Light blue sky
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Read input for this frame
    sceCtrlPeekBufferPositive(0, &g_vita_ctrl, 1);
    
    // Update asset streaming based on performance
    if (g_perf_stats.frame_count % 60 == 0) { // Every second
        AC_Vita_Unload_Unused_Assets();
    }
}

void AC_Vita_Frame_End(void) {
    if (!g_frame_active) return;
    
    // Finish VitaGL frame
    vglStopRenderingInit();
    vglStopRenderingTerm();
    glFinish();
    
    // Update performance stats
    g_perf_stats.frame_count++;
    g_frame_active = false;
    
    // Print stats every 5 seconds in debug mode
    #ifdef AC_DEBUG_PERFORMANCE
    if (g_perf_stats.frame_count % 300 == 0) {
        AC_Vita_Print_Performance_Stats();
    }
    #endif
}

// =============================================================================
// GRAPHICS SYSTEM IMPLEMENTATION (GX -> VitaGL)
// =============================================================================

void GXInit(void* base, u32 size) {
    // Already handled in AC_Vita_Platform_Init
    printf("📺 GXInit() -> VitaGL (already initialized)\n");
}

void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ) {
    glViewport((GLint)left, (GLint)top, (GLsizei)width, (GLsizei)height);
    
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // GameCube uses different coordinate system, adjust
    float aspect = width / height;
    glFrustum(-aspect * nearZ, aspect * nearZ, -nearZ, nearZ, nearZ, farZ);
    
    glMatrixMode(GL_MODELVIEW);
}

void GXSetScissor(u32 left, u32 top, u32 width, u32 height) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, top, width, height);
}

void GXBegin(GXPrimitive type, u32 vtxfmt, u16 nverts) {
    g_current_primitive = type;
    g_vertex_count = nverts;
    glBegin(type);
    g_perf_stats.draw_calls++;
}

void GXEnd(void) {
    glEnd();
}

void GXPosition3f32(f32 x, f32 y, f32 z) {
    glVertex3f(x, y, z);
}

void GXPosition2f32(f32 x, f32 y) {
    glVertex2f(x, y);
}

void GXNormal3f32(f32 x, f32 y, f32 z) {
    glNormal3f(x, y, z);
}

void GXColor4u8(u8 r, u8 g, u8 b, u8 a) {
    glColor4ub(r, g, b, a);
}

void GXTexCoord2f32(f32 s, f32 t) {
    glTexCoord2f(s, t);
}

void GXLoadIdentity(void) {
    glLoadIdentity();
}

void GXLoadPosMtxImm(f32 mtx[3][4], u32 pnmtx) {
    // Convert GameCube 3x4 matrix to OpenGL 4x4 matrix
    GLfloat gl_matrix[16] = {
        mtx[0][0], mtx[1][0], mtx[2][0], 0.0f,
        mtx[0][1], mtx[1][1], mtx[2][1], 0.0f,
        mtx[0][2], mtx[1][2], mtx[2][2], 0.0f,
        mtx[0][3], mtx[1][3], mtx[2][3], 1.0f
    };
    glLoadMatrixf(gl_matrix);
}

void GXSetCurrentMtx(u32 mtxid) {
    // Matrix selection is handled automatically in OpenGL
}

void GXLoadTexObj(void* obj, u32 mapid) {
    // In AC-Decomp, texture objects contain texture data
    // We'll load the texture and bind it
    if (obj) {
        g_current_texture = ac_load_texture_runtime("auto_texture");
        glBindTexture(GL_TEXTURE_2D, g_current_texture);
    }
}

void GXInitTexObj(void* obj, void* img_ptr, u16 width, u16 height, 
                  GXTexFmt format, u32 wrap_s, u32 wrap_t, u32 mipmap) {
    if (!obj || !img_ptr) return;
    
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    
    // Convert GX format to OpenGL format
    GLenum gl_format = GL_RGBA;
    GLenum gl_type = GL_UNSIGNED_BYTE;
    
    switch (format) {
        case GX_RGBA8: gl_format = GL_RGBA; gl_type = GL_UNSIGNED_BYTE; break;
        case GX_RGB8: gl_format = GL_RGB; gl_type = GL_UNSIGNED_BYTE; break;
        case GX_RGB565: gl_format = GL_RGB; gl_type = GL_UNSIGNED_SHORT_5_6_5; break;
        case GX_RGBA4: gl_format = GL_RGBA; gl_type = GL_UNSIGNED_SHORT_4_4_4_4; break;
        case GX_IA8: gl_format = GL_LUMINANCE_ALPHA; gl_type = GL_UNSIGNED_BYTE; break;
        case GX_I8: gl_format = GL_LUMINANCE; gl_type = GL_UNSIGNED_BYTE; break;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, gl_format, width, height, 0, gl_format, gl_type, img_ptr);
    
    // Set wrap modes
    GLenum gl_wrap_s = (wrap_s == 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    GLenum gl_wrap_t = (wrap_t == 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap_t);
    
    // Set filtering
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    // Store texture ID in the object (assuming first 4 bytes)
    *(GLuint*)obj = tex_id;
}

void GXSetCopyClear(GXColor clear_clr, u32 clear_z) {
    glClearColor(clear_clr.r / 255.0f, clear_clr.g / 255.0f, 
                 clear_clr.b / 255.0f, clear_clr.a / 255.0f);
    glClearDepth(clear_z / 16777215.0f); // 24-bit to float
}

void GXCopyDisp(void* dest, u32 clear) {
    // Frame buffer copy - handled automatically by VitaGL
    if (clear) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void GXDrawDone(void) {
    glFlush();
}

// =============================================================================
// INPUT SYSTEM IMPLEMENTATION (PAD -> sceCtrl)
// =============================================================================

u32 PADRead(PADStatus* status) {
    if (!status) return 0;
    
    // Clear status
    memset(status, 0, sizeof(PADStatus));
    
    // Map Vita buttons to GameCube buttons
    u16 buttons = 0;
    if (g_vita_ctrl.buttons & SCE_CTRL_CROSS) buttons |= PAD_BUTTON_A;
    if (g_vita_ctrl.buttons & SCE_CTRL_CIRCLE) buttons |= PAD_BUTTON_B;
    if (g_vita_ctrl.buttons & SCE_CTRL_SQUARE) buttons |= PAD_BUTTON_X;
    if (g_vita_ctrl.buttons & SCE_CTRL_TRIANGLE) buttons |= PAD_BUTTON_Y;
    if (g_vita_ctrl.buttons & SCE_CTRL_START) buttons |= PAD_BUTTON_START;
    if (g_vita_ctrl.buttons & SCE_CTRL_UP) buttons |= PAD_BUTTON_UP;
    if (g_vita_ctrl.buttons & SCE_CTRL_DOWN) buttons |= PAD_BUTTON_DOWN;
    if (g_vita_ctrl.buttons & SCE_CTRL_LEFT) buttons |= PAD_BUTTON_LEFT;
    if (g_vita_ctrl.buttons & SCE_CTRL_RIGHT) buttons |= PAD_BUTTON_RIGHT;
    if (g_vita_ctrl.buttons & SCE_CTRL_LTRIGGER) buttons |= PAD_TRIGGER_L;
    if (g_vita_ctrl.buttons & SCE_CTRL_RTRIGGER) buttons |= PAD_TRIGGER_R;
    
    status->button = buttons;
    
    // Map analog stick (0-255 Vita -> -100 to +100 GameCube)
    status->stickX = (s8)((g_vita_ctrl.lx - 128) * PAD_STICK_RANGE / 128);
    status->stickY = (s8)((g_vita_ctrl.ly - 128) * PAD_STICK_RANGE / 128);
    status->substickX = (s8)((g_vita_ctrl.rx - 128) * PAD_STICK_RANGE / 128);
    status->substickY = (s8)((g_vita_ctrl.ry - 128) * PAD_STICK_RANGE / 128);
    
    status->err = 0; // No error
    return 1; // Success
}

void PADControlMotor(s32 chan, u32 command) {
    // Vita doesn't have controller motors, but we could use LED or something
    // For now, just ignore
}

// =============================================================================
// AUDIO SYSTEM IMPLEMENTATION
// =============================================================================

void AXInit(void) {
    // Already handled in AC_Vita_Platform_Init
}

void AXQuit(void) {
    // Handled in cleanup
}

u32 AXGetInputSamples(void) {
    return 1024; // Return our buffer size
}

void AXRegisterAuxACallback(AXAuxCallback callback, void* context) {
    // Store callback for later use
    // For now, just acknowledge
}

// =============================================================================
// MEMORY MANAGEMENT IMPLEMENTATION
// =============================================================================

static void* g_heap_base = NULL;
static size_t g_heap_size = 0;

void* OSAllocFromHeap(s32 heap, u32 size) {
    return malloc(size);
}

void OSFreeToHeap(s32 heap, void* ptr) {
    free(ptr);
}

s32 OSCreateHeap(void* start, void* end) {
    g_heap_base = start;
    g_heap_size = (char*)end - (char*)start;
    return 0; // Return heap ID
}

void DCFlushRange(void* startAddr, u32 nBytes) {
    // Cache operations - not strictly necessary on Vita but can be no-ops
}

void DCInvalidateRange(void* startAddr, u32 nBytes) {
    // Cache operations
}

void DCStoreRange(void* startAddr, u32 nBytes) {
    // Cache operations
}

void ICInvalidateRange(void* startAddr, u32 nBytes) {
    // Instruction cache operations
}

// =============================================================================
// TIME SYSTEM IMPLEMENTATION
// =============================================================================

OSTime OSGetTime(void) {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return (OSTime)tick.tick;
}

u32 OSTicksToMilliseconds(OSTime ticks) {
    return (u32)(ticks / 1000);
}

u32 OSTicksToMicroseconds(OSTime ticks) {
    return (u32)ticks;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void AC_Vita_Load_All_Assets_For_Scene(const char* scene_name) {
    printf("🎬 Loading assets for scene: %s\n", scene_name);
    
    // Preload common assets based on scene
    if (strcmp(scene_name, "title") == 0) {
        ac_preload_category("ui");
    } else if (strcmp(scene_name, "game") == 0) {
        ac_preload_category("environment");
        ac_preload_category("characters");
    } else if (strcmp(scene_name, "house") == 0) {
        ac_preload_category("items");
    }
    
    g_perf_stats.assets_loaded++;
}

void AC_Vita_Unload_Unused_Assets(void) {
    // Let the asset bridge handle this
    ac_bridge_cleanup_unused();
}

void AC_Vita_Print_Performance_Stats(void) {
    OSTime current_time = OSGetTime();
    u32 elapsed_ms = OSTicksToMilliseconds(current_time - g_perf_stats.frame_start_time);
    
    printf("📊 Performance Stats (Frame %u):\n", g_perf_stats.frame_count);
    printf("   ⏱️  FPS: %.1f\n", (float)g_perf_stats.frame_count * 1000.0f / elapsed_ms);
    printf("   🎨 Draw calls this frame: %u\n", g_perf_stats.draw_calls);
    printf("   📦 Assets loaded: %u\n", g_perf_stats.assets_loaded);
    
    // Print asset bridge stats
    ac_bridge_print_stats();
} 