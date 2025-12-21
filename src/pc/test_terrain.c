/**
 * @file test_terrain.c
 * @brief Test terrain display list for PC port
 *
 * Provides a simple procedural terrain display list to verify the GBI
 * rendering pipeline works before integrating the full field system.
 *
 * NOTE: Display lists are allocated on the HEAP at runtime to ensure
 * addresses are in the same range as g_heap_base, allowing proper
 * 64-bit pointer recovery in the GBI interpreter.
 */

#include "pc/gbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/* Heap-allocated terrain data (allocated at runtime) */
static Vtx* test_terrain_vtx = NULL;
static Gfx* test_terrain_dl = NULL;

static bool test_terrain_initialized = false;

/* Build display list at runtime to get correct 64-bit pointers */
static void test_terrain_init(void) {
    Gfx* dl;

    /* Allocate on heap to ensure addresses match g_heap_base */
    test_terrain_vtx = (Vtx*)malloc(4 * sizeof(Vtx));
    test_terrain_dl = (Gfx*)malloc(12 * sizeof(Gfx));  /* Extra room for texture disable */

    if (!test_terrain_vtx || !test_terrain_dl) {
        printf("[TEST_TERRAIN] ERROR: Failed to allocate memory!\n");
        return;
    }

    dl = test_terrain_dl;

    /* Initialize vertices - green ground plane, 1024x1024 units */
    /* Front-left */
    test_terrain_vtx[0].v.ob[0] = -512; test_terrain_vtx[0].v.ob[1] = 0; test_terrain_vtx[0].v.ob[2] = -512;
    test_terrain_vtx[0].v.flag = 0;
    test_terrain_vtx[0].v.tc[0] = 0; test_terrain_vtx[0].v.tc[1] = 0;
    test_terrain_vtx[0].v.cn[0] = 0; test_terrain_vtx[0].v.cn[1] = 180; test_terrain_vtx[0].v.cn[2] = 0; test_terrain_vtx[0].v.cn[3] = 255;

    /* Front-right */
    test_terrain_vtx[1].v.ob[0] = 512; test_terrain_vtx[1].v.ob[1] = 0; test_terrain_vtx[1].v.ob[2] = -512;
    test_terrain_vtx[1].v.flag = 0;
    test_terrain_vtx[1].v.tc[0] = 0; test_terrain_vtx[1].v.tc[1] = 0;
    test_terrain_vtx[1].v.cn[0] = 0; test_terrain_vtx[1].v.cn[1] = 200; test_terrain_vtx[1].v.cn[2] = 0; test_terrain_vtx[1].v.cn[3] = 255;

    /* Back-right */
    test_terrain_vtx[2].v.ob[0] = 512; test_terrain_vtx[2].v.ob[1] = 0; test_terrain_vtx[2].v.ob[2] = 512;
    test_terrain_vtx[2].v.flag = 0;
    test_terrain_vtx[2].v.tc[0] = 0; test_terrain_vtx[2].v.tc[1] = 0;
    test_terrain_vtx[2].v.cn[0] = 0; test_terrain_vtx[2].v.cn[1] = 160; test_terrain_vtx[2].v.cn[2] = 0; test_terrain_vtx[2].v.cn[3] = 255;

    /* Back-left */
    test_terrain_vtx[3].v.ob[0] = -512; test_terrain_vtx[3].v.ob[1] = 0; test_terrain_vtx[3].v.ob[2] = 512;
    test_terrain_vtx[3].v.flag = 0;
    test_terrain_vtx[3].v.tc[0] = 0; test_terrain_vtx[3].v.tc[1] = 0;
    test_terrain_vtx[3].v.cn[0] = 0; test_terrain_vtx[3].v.cn[1] = 140; test_terrain_vtx[3].v.cn[2] = 0; test_terrain_vtx[3].v.cn[3] = 255;

    /* Build display list at runtime */

    /* Disable texturing - use vertex colors instead of prim_color */
    /* G_TEXTURE: w0 = cmd | level<<11 | tile<<8 | on<<1, w1 = scale_s<<16 | scale_t */
    dl->words.w0 = _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(0, 11, 3) | _SHIFTL(0, 8, 3) | _SHIFTL(G_OFF, 1, 7);
    dl->words.w1 = 0;  /* Scale doesn't matter when off */
    dl++;

    /* Clear lighting mode so vertex colors are used */
    /* Format: w0 = G_GEOMETRYMODE | ~clear_mode, w1 = 0 */
    dl->words.w0 = _SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)G_LIGHTING, 0, 24);
    dl->words.w1 = 0;
    dl++;

    /* Enable back-face culling */
    /* Format: w0 = G_GEOMETRYMODE | 0x00FFFFFF, w1 = set_mode */
    dl->words.w0 = _SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~0, 0, 24);
    dl->words.w1 = G_CULL_BACK;
    dl++;

    /* Load 4 vertices at index 0 */
    /* G_VTX: word0 = cmd | (n << 12) | ((v0+n) << 1), word1 = address */
    dl->words.w0 = _SHIFTL(G_VTX, 24, 8) | _SHIFTL(4, 12, 8) | _SHIFTL(4, 1, 7);
    dl->words.w1 = (u32)(uintptr_t)test_terrain_vtx;  /* Lower 32 bits - will be resolved by seg2ptr */
    dl++;

    /* Draw two triangles: (0,1,2) and (0,2,3) */
    /* Use 0xBE (F3D G_TRI2) format: word0 = cmd | (v0*2 << 16) | (v1*2 << 8) | (v2*2), word1 = (v3*2 << 16) | (v4*2 << 8) | (v5*2) */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(0*2, 16, 8) | _SHIFTL(1*2, 8, 8) | _SHIFTL(2*2, 0, 8);
    dl->words.w1 = _SHIFTL(0*2, 16, 8) | _SHIFTL(2*2, 8, 8) | _SHIFTL(3*2, 0, 8);
    dl++;

    /* End display list */
    dl->words.w0 = _SHIFTL(G_ENDDL, 24, 8);
    dl->words.w1 = 0;

    test_terrain_initialized = true;
    printf("[TEST_TERRAIN] Initialized test terrain display list at %p, vtx at %p\n",
           (void*)test_terrain_dl, (void*)test_terrain_vtx);
}

/* Store full 64-bit pointer for direct access by GBI interpreter */
static void* g_test_terrain_dl_fullptr = NULL;

/* Real terrain from real_terrain.c */
extern Gfx* real_terrain_get_dl(void);
extern void* real_terrain_get_fullptr(u32 truncated_addr);

/* Binary terrain from terrain_loader.c */
extern Gfx* terrain_get_loaded_dl(void);

/* Toggle: 0=test squares, 1=runtime terrain (real_terrain.c), 2=binary terrain */
#define USE_TERRAIN_MODE 1

/* Return terrain display list for specified block */
void* test_terrain_get_display_list(int bx, int bz) {
    (void)bx; (void)bz;  /* Ignore block coords for now */

#if USE_TERRAIN_MODE == 2
    /* Use binary-loaded terrain */
    {
        static int call_count = 0;
        Gfx* bin_dl = terrain_get_loaded_dl();
        if (bin_dl) {
            if (call_count < 5) {
                printf("[TEST_TERRAIN] Returning BINARY terrain dl=%p (lower32=0x%08X) call#%d\n",
                       (void*)bin_dl, (u32)(uintptr_t)bin_dl, call_count);
                call_count++;
            }
            return bin_dl;
        } else {
            static int null_logged = 0;
            if (null_logged < 3) {
                printf("[TEST_TERRAIN] binary terrain NULL, falling through\n");
                null_logged++;
            }
        }
    }
    /* Fall through to runtime terrain if binary not available */
#endif

#if USE_TERRAIN_MODE >= 1
    /* Use real grd_s_c1_1 terrain data (runtime-built) */
    {
        static int rt_logged = 0;
        Gfx* real_dl = real_terrain_get_dl();
        if (!rt_logged && real_dl) {
            printf("[TEST_TERRAIN] Returning REAL terrain dl=%p (lower32=0x%08X)\n",
                   (void*)real_dl, (u32)(uintptr_t)real_dl);
            rt_logged = 1;
        }
        return real_dl;
    }
#else
    /* Use test squares */
    if (!test_terrain_initialized) {
        test_terrain_init();
        g_test_terrain_dl_fullptr = test_terrain_dl;
    }
    return test_terrain_dl;
#endif
}

/* Called by GBI interpreter to get full 64-bit pointer for test terrain display list */
void* test_terrain_get_fullptr(u32 truncated_addr) {
#if USE_TERRAIN_MODE >= 1
    /* Check real terrain first */
    void* real_ptr = real_terrain_get_fullptr(truncated_addr);
    if (real_ptr != NULL) {
        return real_ptr;
    }
#endif
    /* Fall back to test terrain */
    if (g_test_terrain_dl_fullptr != NULL &&
        (u32)(uintptr_t)g_test_terrain_dl_fullptr == truncated_addr) {
        return g_test_terrain_dl_fullptr;
    }
    return NULL;
}

/* Called by GBI interpreter to get full 64-bit pointer for test terrain vertices */
void* test_terrain_get_vtx_fullptr(u32 truncated_addr) {
#if USE_TERRAIN_MODE >= 1
    /* Check real terrain first */
    void* real_ptr = real_terrain_get_fullptr(truncated_addr);
    if (real_ptr != NULL) {
        return real_ptr;
    }
#endif
    /* Fall back to test terrain */
    if (test_terrain_vtx != NULL &&
        (u32)(uintptr_t)test_terrain_vtx == truncated_addr) {
        return test_terrain_vtx;
    }
    return NULL;
}

/* ============================================================================
 * Player Cube Placeholder
 * ============================================================================
 * A simple blue cube drawn at the player position. Uses BG_OPA buffer
 * (same as terrain) to ensure it gets rendered.
 */

#include "graph.h"  /* For OPEN_DISP/CLOSE_DISP macros */

static Vtx* player_cube_vtx = NULL;
static Gfx* player_cube_dl = NULL;
static bool player_cube_initialized = false;

static void player_cube_init(void) {
    Gfx* dl;
    int i;

    /* Allocate on heap */
    player_cube_vtx = (Vtx*)malloc(8 * sizeof(Vtx));
    player_cube_dl = (Gfx*)malloc(20 * sizeof(Gfx));

    if (!player_cube_vtx || !player_cube_dl) {
        printf("[PLAYER_CUBE] ERROR: Failed to allocate memory!\n");
        return;
    }

    /* 8 vertices for cube - 2500 units wide x 3000 tall (~40x48 pixels)
     * Will be positioned via player_cube_set_world_position() each frame */
    i = 0;
    player_cube_vtx[i].v.ob[0] = -1250; player_cube_vtx[i].v.ob[1] = 0;    player_cube_vtx[i].v.ob[2] = -1250;
    player_cube_vtx[i].v.cn[0] = 50;  player_cube_vtx[i].v.cn[1] = 50;  player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = 1250;  player_cube_vtx[i].v.ob[1] = 0;    player_cube_vtx[i].v.ob[2] = -1250;
    player_cube_vtx[i].v.cn[0] = 50;  player_cube_vtx[i].v.cn[1] = 50;  player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = 1250;  player_cube_vtx[i].v.ob[1] = 0;    player_cube_vtx[i].v.ob[2] = 1250;
    player_cube_vtx[i].v.cn[0] = 50;  player_cube_vtx[i].v.cn[1] = 50;  player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = -1250; player_cube_vtx[i].v.ob[1] = 0;    player_cube_vtx[i].v.ob[2] = 1250;
    player_cube_vtx[i].v.cn[0] = 50;  player_cube_vtx[i].v.cn[1] = 50;  player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = -1250; player_cube_vtx[i].v.ob[1] = 3000; player_cube_vtx[i].v.ob[2] = -1250;
    player_cube_vtx[i].v.cn[0] = 100; player_cube_vtx[i].v.cn[1] = 100; player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = 1250;  player_cube_vtx[i].v.ob[1] = 3000; player_cube_vtx[i].v.ob[2] = -1250;
    player_cube_vtx[i].v.cn[0] = 100; player_cube_vtx[i].v.cn[1] = 100; player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = 1250;  player_cube_vtx[i].v.ob[1] = 3000; player_cube_vtx[i].v.ob[2] = 1250;
    player_cube_vtx[i].v.cn[0] = 100; player_cube_vtx[i].v.cn[1] = 100; player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    player_cube_vtx[i].v.ob[0] = -1250; player_cube_vtx[i].v.ob[1] = 3000; player_cube_vtx[i].v.ob[2] = 1250;
    player_cube_vtx[i].v.cn[0] = 100; player_cube_vtx[i].v.cn[1] = 100; player_cube_vtx[i].v.cn[2] = 255; player_cube_vtx[i++].v.cn[3] = 255;

    /* Build display list */
    dl = player_cube_dl;

    /* Disable texturing - use vertex colors */
    dl->words.w0 = _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(0, 11, 3) | _SHIFTL(0, 8, 3) | _SHIFTL(G_OFF, 1, 7);
    dl->words.w1 = 0;
    dl++;

    /* Disable lighting to use vertex colors */
    dl->words.w0 = _SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)G_LIGHTING, 0, 24);
    dl->words.w1 = 0;
    dl++;

    /* Load 8 vertices */
    dl->words.w0 = _SHIFTL(G_VTX, 24, 8) | _SHIFTL(8, 12, 8) | _SHIFTL(8, 1, 7);
    dl->words.w1 = (u32)(uintptr_t)player_cube_vtx;
    dl++;

    /* Draw 6 faces (12 triangles) using G_TRI2 */
    /* Bottom: 0,1,2 and 0,2,3 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(0*2, 16, 8) | _SHIFTL(2*2, 8, 8) | _SHIFTL(1*2, 0, 8);
    dl->words.w1 = _SHIFTL(0*2, 16, 8) | _SHIFTL(3*2, 8, 8) | _SHIFTL(2*2, 0, 8);
    dl++;

    /* Top: 4,5,6 and 4,6,7 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(4*2, 16, 8) | _SHIFTL(5*2, 8, 8) | _SHIFTL(6*2, 0, 8);
    dl->words.w1 = _SHIFTL(4*2, 16, 8) | _SHIFTL(6*2, 8, 8) | _SHIFTL(7*2, 0, 8);
    dl++;

    /* Front: 0,1,5 and 0,5,4 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(0*2, 16, 8) | _SHIFTL(1*2, 8, 8) | _SHIFTL(5*2, 0, 8);
    dl->words.w1 = _SHIFTL(0*2, 16, 8) | _SHIFTL(5*2, 8, 8) | _SHIFTL(4*2, 0, 8);
    dl++;

    /* Back: 2,3,7 and 2,7,6 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(2*2, 16, 8) | _SHIFTL(3*2, 8, 8) | _SHIFTL(7*2, 0, 8);
    dl->words.w1 = _SHIFTL(2*2, 16, 8) | _SHIFTL(7*2, 8, 8) | _SHIFTL(6*2, 0, 8);
    dl++;

    /* Left: 0,3,7 and 0,7,4 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(0*2, 16, 8) | _SHIFTL(3*2, 8, 8) | _SHIFTL(7*2, 0, 8);
    dl->words.w1 = _SHIFTL(0*2, 16, 8) | _SHIFTL(7*2, 8, 8) | _SHIFTL(4*2, 0, 8);
    dl++;

    /* Right: 1,2,6 and 1,6,5 */
    dl->words.w0 = _SHIFTL(0xBE, 24, 8) | _SHIFTL(1*2, 16, 8) | _SHIFTL(2*2, 8, 8) | _SHIFTL(6*2, 0, 8);
    dl->words.w1 = _SHIFTL(1*2, 16, 8) | _SHIFTL(6*2, 8, 8) | _SHIFTL(5*2, 0, 8);
    dl++;

    /* End display list */
    dl->words.w0 = _SHIFTL(G_ENDDL, 24, 8);
    dl->words.w1 = 0;

    player_cube_initialized = true;
    printf("[PLAYER_CUBE] Initialized player cube at dl=%p vtx=%p\n",
           (void*)player_cube_dl, (void*)player_cube_vtx);
}

/* Pointer lookup for GBI interpreter */
void* player_cube_dl_get_fullptr(u32 truncated_addr) {
    if (player_cube_dl != NULL &&
        (u32)(uintptr_t)player_cube_dl == truncated_addr) {
        return player_cube_dl;
    }
    return NULL;
}

void* player_cube_vtx_get_fullptr(u32 truncated_addr) {
    if (player_cube_vtx != NULL &&
        (u32)(uintptr_t)player_cube_vtx == truncated_addr) {
        return player_cube_vtx;
    }
    return NULL;
}

/**
 * Update player cube world position.
 * Updates vertex positions to render cube at given world coordinates.
 * Call this each frame BEFORE calling player_cube_get_dl().
 */
void player_cube_set_world_position(f32 x, f32 y, f32 z) {
    static f32 last_x = 0, last_y = 0, last_z = 0;
    static int pos_debug = 0;
    s16 ix, iy, iz;
    int i;

    /* Base cube offsets from center (relative vertices)
     * ~63 units = 1 screen pixel (based on 1000 units -> 15.6 px)
     * Make cube 40x60 pixels = 2500x3780 units */
    static const s16 base_vtx[8][3] = {
        {-1250, 0,    -1250},  /* 0: Bottom front-left */
        { 1250, 0,    -1250},  /* 1: Bottom front-right */
        { 1250, 0,     1250},  /* 2: Bottom back-right */
        {-1250, 0,     1250},  /* 3: Bottom back-left */
        {-1250, 3000, -1250},  /* 4: Top front-left */
        { 1250, 3000, -1250},  /* 5: Top front-right */
        { 1250, 3000,  1250},  /* 6: Top back-right */
        {-1250, 3000,  1250},  /* 7: Top back-left */
    };

    if (!player_cube_initialized || !player_cube_vtx) {
        return;
    }

    /* Convert world position to fixed-point with reasonable scale */
    /* N64 uses large world coords (5120 = center of 10240 world) */
    /* We need to offset the cube by the player's position relative to view */
    ix = (s16)(x - 5120.0f);  /* Center of world is 5120 */
    iy = (s16)(y);
    iz = (s16)(z - 5120.0f);

    /* Only log when position changes significantly */
    if (pos_debug < 10 ||
        (x != last_x || y != last_y || z != last_z)) {
        if (pos_debug < 10) {
            printf("[CUBE_POS] Setting world pos (%.1f, %.1f, %.1f) -> offset (%d, %d, %d)\n",
                   x, y, z, ix, iy, iz);
            fflush(stdout);
            pos_debug++;
        }
        last_x = x; last_y = y; last_z = z;
    }

    /* Update all 8 vertices with new position offset */
    for (i = 0; i < 8; i++) {
        player_cube_vtx[i].v.ob[0] = base_vtx[i][0] + ix;
        player_cube_vtx[i].v.ob[1] = base_vtx[i][1] + iy;
        player_cube_vtx[i].v.ob[2] = base_vtx[i][2] + iz;
    }
}

/**
 * Get the player cube display list pointer.
 * Initializes the cube if not already done.
 * This is for direct insertion into BG_OPA without OPEN/CLOSE_DISP overhead.
 */
Gfx* player_cube_get_dl(void) {
    if (!player_cube_initialized) {
        player_cube_init();
    }
    return player_cube_dl;
}

/**
 * Draw player cube placeholder at given world position.
 * Uses BG_OPA buffer (same as terrain) which is proven to work.
 *
 * @param graph The GRAPH context to use for display list output
 * @param px, py, pz World position (currently unused - cube at origin)
 */
void player_cube_draw(GRAPH* graph, f32 px, f32 py, f32 pz) {
    static int draw_debug = 0;

    (void)px; (void)py; (void)pz;  /* TODO: Add translation matrix */

    if (!player_cube_initialized) {
        player_cube_init();
    }

    if (!player_cube_dl) {
        return;
    }

    OPEN_DISP(graph);

    if (draw_debug < 5) {
        printf("[PLAYER_CUBE_DRAW] Adding cube to BG_OPA:\n");
        printf("  dl=%p (trunc=0x%08X)\n",
               (void*)player_cube_dl, (u32)(uintptr_t)player_cube_dl);
        printf("  BG_OPA head BEFORE=%p\n", (void*)NOW_BG_OPA_DISP);
        fflush(stdout);
    }

    /* Add cube display list to BG_OPA buffer (same as terrain) */
    gSPDisplayList(NEXT_BG_OPA_DISP, player_cube_dl);

    if (draw_debug < 5) {
        printf("  BG_OPA head AFTER=%p\n", (void*)NOW_BG_OPA_DISP);
        fflush(stdout);
        draw_debug++;
    }

    CLOSE_DISP(graph);
}
