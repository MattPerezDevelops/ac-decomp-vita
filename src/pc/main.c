/**
 * @file main.c
 * @brief PC port entry point
 *
 * This is the main entry point for the Animal Crossing PC port.
 * It initializes SDL, creates the window, and runs the game.
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>

#include "pc/platform.h"
#include "pc/os_compat.h"
#include "pc/gbi.h"  /* GBI types and commands */

/* External declarations for PC port systems */
extern void window_init(int width, int height, const char* title);
extern void window_shutdown(void);
extern void window_swap_buffers(void);
extern int window_should_close(void);
extern void window_poll_events(void);

extern void input_init(void);
extern void input_update(void);
extern void input_shutdown(void);

extern void audio_init(void);
extern void audio_shutdown(void);

/* External declaration for the actual game entry point */
extern void graph_proc(void* arg);

/* External declarations for DVD init */
extern void DVDInit(void);

/* External declarations for archive loading */
extern void JW_Init2(void);
extern void JW_Init3(void);

/* Game state */
static int g_running = 1;
int g_frame_count = 0;

/* JSystem frame hooks - called by graph.c via JW_BeginFrame/JW_EndFrame */
void JW_BeginFrame(void) {
    static Uint32 last_time = 0;
    static int fps_counter = 0;

    /* Poll SDL events at frame start */
    window_poll_events();
    input_update();

    /* Check for quit */
    if (window_should_close()) {
        g_running = 0;
    }

    /* FPS tracking */
    fps_counter++;
    Uint32 now = SDL_GetTicks();
    if (now - last_time >= 1000) {
        printf("[FPS] %d frames in last second\n", fps_counter);
        fflush(stdout);
        fps_counter = 0;
        last_time = now;
    }

    /* Clear screen at frame start */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  /* Black background */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Set up basic OpenGL state - use same projection as gbi_execute
     * Y range is 0-640 to match GBI coordinate system, though window is 480 tall */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 640, 640, 0, -1, 1);  /* Match GBI interpreter projection */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void JW_EndFrame(void) {
    static int endframe_debug = 0;
    if (endframe_debug < 10) {
        printf("[JW_EndFrame] Called frame=%d\n", g_frame_count);
        fflush(stdout);
        endframe_debug++;
    }

    /* Phase 1 Diagnostic: Identity Triangle Test
     * Draw a magenta triangle using pure OpenGL to verify the rendering pipeline works.
     * If this appears on screen, OpenGL is working and the issue is in GBI coordinate handling.
     * DISABLED: GBI coordinate system fixed - using clip coords with Y-flip now. */
#if 0  /* Disabled - GBI fixes complete */
    {
        extern void gbi_test_identity_triangle(void);
        /* Only draw for first 300 frames to verify, then disable to see terrain */
        if (g_frame_count < 300) {
            gbi_test_identity_triangle();
        }
    }
#endif

    /* Debug test: draw dump model vertices directly to verify binary data */
#if 0  /* Disable after verifying model loader works */
    {
        extern Vtx* model_loader_get_dump_vertices(void);
        Vtx* verts = model_loader_get_dump_vertices();
        static int dump_debug = 0;

        if (verts && g_frame_count > 100) {  /* After logo fades */
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            /* Scale to fit dump coords (-12000 to +12000) into screen */
            glOrtho(-15000, 15000, -15000, 15000, -10000, 10000);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            /* Draw dump vertices as points and lines */
            glColor4f(0.0f, 1.0f, 0.0f, 1.0f);  /* Green */
            glPointSize(5.0f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 68; i++) {
                float x = verts[i].v.ob[0];
                float y = verts[i].v.ob[1];
                float z = verts[i].v.ob[2];
                glVertex3f(x, y, z);
            }
            glEnd();

            /* Draw as triangles (quads) */
            glColor4f(0.2f, 0.6f, 0.2f, 0.5f);  /* Translucent green */
            glBegin(GL_QUADS);
            /* First 16 vertices are the fence posts (4 quads) */
            for (int i = 0; i < 16; i += 4) {
                for (int j = 0; j < 4; j++) {
                    glVertex3f(verts[i+j].v.ob[0], verts[i+j].v.ob[1], verts[i+j].v.ob[2]);
                }
            }
            glEnd();

            if (dump_debug < 3) {
                printf("[DEBUG_DUMP] Drawing dump vertices, first vertex: (%d,%d,%d)\n",
                       verts[0].v.ob[0], verts[0].v.ob[1], verts[0].v.ob[2]);
                dump_debug++;
            }
        }
    }
#endif

    /* Force commands to complete before swap */
    glFlush();

    /* Check for GL errors */
    GLenum err = glGetError();
    if (err != GL_NO_ERROR && g_frame_count < 5) {
        printf("GL Error in JW_EndFrame: 0x%04X\n", err);
    }

    /* Swap buffers at frame end */
    window_swap_buffers();

    g_frame_count++;

    /* Print frame count periodically */
    if (g_frame_count % 60 == 0) {
        printf("Frame: %d\n", g_frame_count);
    }
}

/* Check if game should continue running */
int pc_should_continue(void) {
    return g_running && !window_should_close();
}

/**
 * Program entry point
 */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("Animal Crossing PC Port\n");
    printf("=======================\n");

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    /* Initialize OS compatibility layer */
    osInitialize();

    /* Initialize DVD/file I/O */
    DVDInit();

    /* Load game archives (forest_1st.arc, forest_2nd.arc) */
    JW_Init2();
    JW_Init3();

    /* Create window with OpenGL context */
    window_init(SCREEN_WIDTH, SCREEN_HEIGHT, "Animal Crossing PC");

    /* Initialize GBI interpreter */
    gbi_init();

    /* Initialize pointer registry for 64-bit pointer recovery */
    extern void ptr_registry_init(void);
    extern void ptr_registry_add_range(void* base, size_t size, const char* name);
    extern void terrain_registry_init(void);
    extern void logo_assets_init(void);
    ptr_registry_init();

    /* Register sys_dynamic display list buffers for range-based pointer recovery.
     * These buffers are where game code writes GBI commands, and addresses within
     * them get truncated to 32 bits. By registering the ranges, we can recover
     * the full 64-bit addresses. */
    {
        #include "sys_dynamic.h"
        extern dynamic_t sys_dynamic;

        printf("[MAIN] Registering sys_dynamic buffers for pointer recovery...\n");

        /* Each Gfx is 8 bytes, so multiply element count by sizeof(Gfx) */
        ptr_registry_add_range(sys_dynamic.poly_opa, sizeof(sys_dynamic.poly_opa), "poly_opa");
        ptr_registry_add_range(sys_dynamic.poly_xlu, sizeof(sys_dynamic.poly_xlu), "poly_xlu");
        ptr_registry_add_range(sys_dynamic.overlay, sizeof(sys_dynamic.overlay), "overlay");
        ptr_registry_add_range(sys_dynamic.work, sizeof(sys_dynamic.work), "work");
        ptr_registry_add_range(sys_dynamic.unused, sizeof(sys_dynamic.unused), "unused");
        ptr_registry_add_range(sys_dynamic.font, sizeof(sys_dynamic.font), "font");
        ptr_registry_add_range(sys_dynamic.shadow, sizeof(sys_dynamic.shadow), "shadow");
        ptr_registry_add_range(sys_dynamic.light, sizeof(sys_dynamic.light), "light");
        ptr_registry_add_range(sys_dynamic.new0, sizeof(sys_dynamic.new0), "new0");
        ptr_registry_add_range(sys_dynamic.new1, sizeof(sys_dynamic.new1), "new1");

        printf("[MAIN] sys_dynamic buffers registered\n");
    }

    terrain_registry_init();
    logo_assets_init();

    /* Initialize model loader for actor binary data */
    extern void model_loader_init(void);
    model_loader_init();

    /* Initialize scene loader for 64-bit scene data */
    extern void scene_loader_init(void);
    scene_loader_init();

    /* Set static base for 64-bit pointer recovery from .data/.bss section.
     * Use a known static symbol to derive the base address. */
    extern void gbi_set_static_base(void* ptr);
    extern void graph_proc(void*);  /* Any global function pointer will do */
    gbi_set_static_base((void*)&graph_proc);

    /* Initialize input */
    input_init();

    /* Initialize audio */
    audio_init();

    printf("Initialization complete. Starting game...\n");
    fflush(stdout);

    printf("[MAIN] About to call graph_proc()\n");
    fflush(stdout);

    /* Run the actual game (graph_proc is the game's main entry point) */
    graph_proc(NULL);

    printf("[MAIN] graph_proc() returned\n");
    fflush(stdout);

    /* Cleanup */
    printf("Shutting down...\n");

    audio_shutdown();
    input_shutdown();
    gbi_shutdown();
    window_shutdown();
    pc_osShutdown();

    SDL_Quit();

    printf("Goodbye!\n");
    return 0;
}
