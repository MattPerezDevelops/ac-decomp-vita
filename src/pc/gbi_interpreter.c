/**
 * @file gbi_interpreter.c
 * @brief N64 GBI (Graphics Binary Interface) display list interpreter
 *
 * Part of Layer 3 (Host System Implementation).
 * Parses N64 display list commands and converts them to OpenGL calls.
 *
 * Based on the emu64 system from the GameCube version which provides
 * complete F3DEX2 microcode emulation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#include "pc/gbi.h"  /* GBI types and commands */
#include "pc/texture_registry.h"  /* Texture loading and palette system */

/* Debug output control - set to 0 for release builds */
#ifndef DEBUG_GBI
#define DEBUG_GBI 0
#endif

/* Conditional debug printf */
#if DEBUG_GBI
#define GBI_DEBUG(...) printf(__VA_ARGS__)
#define GBI_DEBUG_FLUSH() fflush(stdout)
#else
#define GBI_DEBUG(...) ((void)0)
#define GBI_DEBUG_FLUSH() ((void)0)
#endif

/* Maximum vertex buffer size (matching N64) */
#define MAX_VERTICES 64
#define MAX_SEGMENTS 16
#define MTX_STACK_SIZE 32

/* Transformed vertex for rendering */
typedef struct {
    f32 x, y, z, w;        /* Transformed position (clip coords) */
    f32 nx, ny, nz;        /* Normal */
    f32 s, t;              /* Texture coords */
    u8 r, g, b, a;         /* Color */
} TransformedVertex;

/* GBI interpreter state - modeled after emu64 */
static struct {
    /* Segment address table (N64 uses segmented memory) */
    uintptr_t segments[MAX_SEGMENTS];

    /* Geometry mode flags */
    u32 geometry_mode;

    /* Matrix stacks - both modelview and projection can be pushed/popped */
    f32 modelview_stack[MTX_STACK_SIZE][4][4];
    int modelview_stack_ptr;
    f32 projection_stack[MTX_STACK_SIZE][4][4];
    int projection_stack_ptr;
    f32 combined_mtx[4][4];
    int combined_mtx_dirty;

    /* Vertex buffer (raw N64 vertices) */
    Vtx vertex_buffer[MAX_VERTICES];

    /* Transformed vertices (after matrix multiply) */
    TransformedVertex transformed[MAX_VERTICES];

    /* Texture state */
    int texture_on;
    u32 texture_tile;
    f32 texture_scale_s;
    f32 texture_scale_t;

    /* Texture image settings */
    u32 timg_addr;
    int timg_fmt;
    int timg_siz;
    int timg_width;
    int timg_height;      /* Dolphin variant stores height */

    /* OpenGL texture state */
    GLuint gl_texture_id;
    int gl_texture_valid;
    int gl_texture_width;
    int gl_texture_height;

    /* RDP other modes */
    u32 othermode_high;
    u32 othermode_low;

    /* Colors */
    u8 prim_color[4];
    u8 env_color[4];
    u8 fog_color[4];
    u8 fill_color[4];
    u8 blend_color[4];

    /* Lights */
    Light lights[8];
    Ambient ambient;
    int num_lights;

    /* Viewport */
    f32 viewport_scale[4];
    f32 viewport_trans[4];

    /* OpenGL viewport (calculated from N64 viewport) */
    struct {
        f32 x, y, width, height;
        int valid;
    } gl_viewport;

    /* Framebuffer */
    void* framebuffer;
    int fb_width;
    int fb_height;

    /* Tile descriptors (N64 RDP has 8 tiles) */
    struct {
        int fmt;        /* Format: G_IM_FMT_RGBA, CI, IA, I, YUV */
        int siz;        /* Size: G_IM_SIZ_4b, 8b, 16b, 32b */
        int line;       /* Line size in 64-bit words */
        int tmem;       /* TMEM address (0-511 words) */
        int pal;        /* Palette number (0-15) */
        int clamps, clampt;
        int mirrors, mirrort;
        int masks, maskt;
        int shifts, shiftt;
        int sl, tl, sh, th;  /* Tile coordinates (10.2 fixed point) */
    } tiles[8];

    /* TMEM simulation (4KB total) */
    u8 tmem[4096];
    int tmem_loaded;  /* Flag if TMEM contains data */

    /* Statistics */
    int triangles_drawn;
    int display_lists_executed;
    int vertices_loaded;
    int matrices_loaded;

} g_gbi_state;

/**
 * Translation offset for positioned objects (player, actors, etc.)
 * Set by gbi_set_translation(), applied in gbi_execute(), cleared afterward.
 */
static struct {
    f32 x, y, z;
    int active;
} g_gbi_translation = { 0, 0, 0, 0 };

/* Debug flags for cube display list tracing */
static int g_in_cube_dl = 0;      /* Flag: are we executing the cube's display list? */
static int g_cube_dl_debug = 0;   /* Limit cube DL debug output */

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Translate segment:offset address to linear address
 * N64 uses segmented addressing; this converts to a direct pointer.
 *
 * On 64-bit systems, pointers stored in display lists are truncated to 32-bit.
 * We attempt to recover the full address using known base addresses.
 */
static uintptr_t g_heap_base = 0;  /* Set during init to recover truncated pointers */

void gbi_set_heap_base(void* base) {
    g_heap_base = (uintptr_t)base & 0xFFFFFFFF00000000ULL;
}

/**
 * Static pointer registration system for display lists in the .bss/.data sections.
 * These can't be recovered using g_heap_base since they're not on the heap.
 */
#define MAX_STATIC_PTRS 64
static struct {
    u32 truncated;   /* Lower 32 bits */
    void* full;      /* Full 64-bit pointer */
} g_static_ptrs[MAX_STATIC_PTRS];
static int g_num_static_ptrs = 0;

void gbi_register_static_ptr(void* ptr) {
    if (ptr == NULL) return;
    if (g_num_static_ptrs >= MAX_STATIC_PTRS) {
        printf("[GBI] ERROR: Too many static pointers registered!\n");
        return;
    }
    u32 truncated = (u32)(uintptr_t)ptr;
    /* Check if already registered */
    for (int i = 0; i < g_num_static_ptrs; i++) {
        if (g_static_ptrs[i].full == ptr) return;
    }
    g_static_ptrs[g_num_static_ptrs].truncated = truncated;
    g_static_ptrs[g_num_static_ptrs].full = ptr;
    g_num_static_ptrs++;
    printf("[GBI] Registered static ptr: %p (0x%08X)\n", ptr, truncated);
}

static int lookup_debug_count = 0;
static void* lookup_static_ptr(u32 addr) {
    for (int i = 0; i < g_num_static_ptrs; i++) {
        if (g_static_ptrs[i].truncated == addr) {
            if (lookup_debug_count++ < 10) {
                /* Print the display list contents when found */
                Gfx* dl = (Gfx*)g_static_ptrs[i].full;
                printf("[LOOKUP] Found static ptr: 0x%08X -> %p\n",
                       addr, g_static_ptrs[i].full);
                printf("  cmd[0]: (0x%08X, 0x%08X)\n", dl[0].words.w0, dl[0].words.w1);
                printf("  cmd[1]: (0x%08X, 0x%08X)\n", dl[1].words.w0, dl[1].words.w1);
                printf("  cmd[2]: (0x%08X, 0x%08X)\n", dl[2].words.w0, dl[2].words.w1);
                fflush(stdout);
            }
            return g_static_ptrs[i].full;
        }
    }
    return NULL;
}

/* Static executable base for recovering pointers in .data/.bss section */
static uintptr_t g_static_base = 0;

void gbi_set_static_base(void* some_ptr) {
    g_static_base = (uintptr_t)some_ptr & 0xFFFFFFFF00000000ULL;
    printf("[GBI] Static base initialized to: 0x%lX (from ptr %p)\n",
           (unsigned long)g_static_base, some_ptr);
}

static int seg2ptr_debug_count = 0;

static void* seg2ptr(u32 addr) {
    u32 segment = (addr >> 24) & 0x0F;
    u32 offset = addr & 0x00FFFFFF;

    if (addr == 0) return NULL;

    /* Check for player cube pointers FIRST - before segment lookup.
     * Heap addresses like 0x3F359070 have non-zero segment bits that could
     * match a set segment and cause early return with garbage. */
    {
        extern void* player_cube_dl_get_fullptr(u32 truncated_addr);
        extern void* player_cube_vtx_get_fullptr(u32 truncated_addr);
        void* player_ptr;

        player_ptr = player_cube_dl_get_fullptr(addr);
        if (player_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> player_cube_dl=%p\n", addr, player_ptr);
            return player_ptr;
        }

        player_ptr = player_cube_vtx_get_fullptr(addr);
        if (player_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> player_cube_vtx=%p\n", addr, player_ptr);
            return player_ptr;
        }
    }

    /* Check for binary terrain pointers BEFORE segment lookup */
    {
        extern void* terrain_bin_get_fullptr(u32 truncated_addr);
        void* bin_ptr = terrain_bin_get_fullptr(addr);
        if (bin_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> terrain_bin=%p\n", addr, bin_ptr);
            return bin_ptr;
        }
    }

    /* Check for test terrain pointers BEFORE segment lookup */
    {
        extern void* test_terrain_get_fullptr(u32 truncated_addr);
        extern void* test_terrain_get_vtx_fullptr(u32 truncated_addr);
        void* test_ptr;

        test_ptr = test_terrain_get_fullptr(addr);
        if (test_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> test_terrain_dl=%p\n", addr, test_ptr);
            return test_ptr;
        }

        test_ptr = test_terrain_get_vtx_fullptr(addr);
        if (test_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> test_terrain_vtx=%p\n", addr, test_ptr);
            return test_ptr;
        }
    }

    /* Check pointer registry (centralized 32->64 bit pointer recovery) */
    {
        extern void* ptr_registry_lookup(u32 truncated);
        void* reg_ptr = ptr_registry_lookup(addr);
        if (reg_ptr != NULL) {
            static int reg_debug = 0;
            if (reg_debug < 10) {
                printf("[SEG2PTR] Registry found 0x%08X -> %p\n", addr, reg_ptr);
                reg_debug++;
            }
            return reg_ptr;
        }
    }

    if (g_gbi_state.segments[segment] != 0) {
        void* result = (void*)(g_gbi_state.segments[segment] + offset);
#if DEBUG_GBI
        if (seg2ptr_debug_count < 30) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X seg=%d offset=0x%06X -> seg_base=0x%lX result=%p\n",
                   addr, segment, offset, (unsigned long)g_gbi_state.segments[segment], result);
            GBI_DEBUG_FLUSH();
            seg2ptr_debug_count++;
        }
#endif
        return result;
    }

    /* No segment set - try to recover truncated 64-bit pointer */
    /* On 64-bit, the display list stores only lower 32 bits of pointer */

    /* First check registered static pointers (display lists in .bss/.data) */
    {
        void* static_ptr = lookup_static_ptr(addr);
        if (static_ptr != NULL) {
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> static_ptr=%p\n", addr, static_ptr);
            return static_ptr;
        }
    }

    /* Try static executable base (for pointers in .data/.bss section).
     * Known static data ranges: sys_dynamic at ~0x829F, logos at ~0x82B1.
     * Addresses like 0x8Cxxxxxx that weren't found in the registry are likely
     * garbage or misinterpreted data - skip them to avoid false recoveries. */
    if (g_static_base != 0) {
        void* recovered = (void*)(g_static_base | (uintptr_t)addr);
        /* Reject addresses in the 0x8C-0x8F range that aren't registered.
         * These cause circular references when falsely recovered. */
        u32 top_nibble = (addr >> 28) & 0xF;
        u32 second_nibble = (addr >> 24) & 0xF;
        if (top_nibble == 0x8 && second_nibble >= 0xC && second_nibble <= 0xF) {
            /* Address like 0x8Cxxxxxx - not in registry, likely garbage */
            static int skip_debug = 0;
            if (skip_debug < 5) {
                printf("[SEG2PTR] Skipping suspicious addr 0x%08X (0x8C-0x8F range not registered)\n", addr);
                skip_debug++;
            }
        } else {
            /* Accept other static addresses */
            GBI_DEBUG("[SEG2PTR] addr=0x%08X -> static_base=0x%lX recovered=%p\n",
                       addr, (unsigned long)g_static_base, recovered);
            return recovered;
        }
    }

    /* Try combining with known heap base to get full address */
    if (g_heap_base != 0) {
        void* recovered = (void*)(g_heap_base | (uintptr_t)addr);
        GBI_DEBUG("[SEG2PTR] addr=0x%08X seg=%d(none) -> heap_base=0x%lX recovered=%p\n",
                   addr, segment, (unsigned long)g_heap_base, recovered);
        /* Basic sanity check - should be in a reasonable range */
        if ((uintptr_t)recovered > 0x1000) {
            return recovered;
        }
    }

    /* Last resort - just return the 32-bit value as pointer (will likely crash) */
    GBI_DEBUG("[SEG2PTR] WARNING: No base to recover addr=0x%08X, returning as-is\n", addr);

    /* Debug: log unrecoverable pointers that look like truncated heap addresses */
    static int unrecov_debug = 0;
    if (unrecov_debug < 20) {
        printf("[SEG2PTR FAIL] addr=0x%08X seg=%d heap_base=0x%lX static_base=0x%lX\n",
               addr, segment, (unsigned long)g_heap_base, (unsigned long)g_static_base);
        fflush(stdout);
        unrecov_debug++;
    }

    return (void*)(uintptr_t)addr;
}

/**
 * Set a segment base address
 */
void gbi_set_segment(int segment, void* addr) {
    if (segment >= 0 && segment < MAX_SEGMENTS) {
        printf("[GBI] gbi_set_segment(%d, %p)\n", segment, addr);
        g_gbi_state.segments[segment] = (uintptr_t)addr;
    }
}

/**
 * Set identity matrix
 */
static void mtx_identity(f32 m[4][4]) {
    memset(m, 0, 16 * sizeof(f32));
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

/**
 * Multiply two 4x4 matrices: result = a * b
 */
static void mtx_multiply(f32 result[4][4], f32 a[4][4], f32 b[4][4]) {
    f32 temp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = a[i][0] * b[0][j] +
                         a[i][1] * b[1][j] +
                         a[i][2] * b[2][j] +
                         a[i][3] * b[3][j];
        }
    }
    memcpy(result, temp, sizeof(temp));
}

/**
 * Convert N64 fixed-point matrix to float matrix
 * N64 matrices use interleaved format (Shipwright's guMtxL2F):
 *   m[0..1] = high 16 bits of each element (rows 0-3)
 *   m[2..3] = low 16 bits of each element (rows 0-3)
 * Values are stitched: (high & 0xFFFF0000) | (low >> 16) for even columns
 *                      (high << 16) | (low & 0xFFFF) for odd columns
 */
static void mtx_n64_to_float(Mtx* n64_mtx, f32 out[4][4]) {
    u32* m1 = (u32*)&n64_mtx->m[0][0];  /* High parts start at m[0] */
    u32* m2 = (u32*)&n64_mtx->m[2][0];  /* Low parts start at m[2] */

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 2; c++) {
            u32 hi = *m1;
            u32 lo = *m2;

            /* Stitch the values together */
            u32 tmp1 = (hi & 0xFFFF0000) | ((lo >> 16) & 0xFFFF);
            u32 tmp2 = ((hi << 16) & 0xFFFF0000) | (lo & 0xFFFF);

            /* Convert to signed and then to float (16.16 fixed point) */
            s32 stmp1 = *(s32*)&tmp1;
            s32 stmp2 = *(s32*)&tmp2;

            out[r][c * 2 + 0] = stmp1 / 65536.0f;
            out[r][c * 2 + 1] = stmp2 / 65536.0f;

            m1++;
            m2++;
        }
    }
}

/**
 * Transform a vertex position by a matrix
 * N64 uses row vectors: result = v * M
 * This is equivalent to result = M^T * v with column vectors
 */
static void transform_vertex(f32 m[4][4], f32 in_x, f32 in_y, f32 in_z,
                             f32* out_x, f32* out_y, f32* out_z, f32* out_w) {
    /* N64 convention: v * M (row vector * matrix) */
    *out_x = in_x * m[0][0] + in_y * m[1][0] + in_z * m[2][0] + 1.0f * m[3][0];
    *out_y = in_x * m[0][1] + in_y * m[1][1] + in_z * m[2][1] + 1.0f * m[3][1];
    *out_z = in_x * m[0][2] + in_y * m[1][2] + in_z * m[2][2] + 1.0f * m[3][2];
    *out_w = in_x * m[0][3] + in_y * m[1][3] + in_z * m[2][3] + 1.0f * m[3][3];
}

/**
 * Update the combined (modelview * projection) matrix
 * N64 uses: screen = vertex * modelview * projection (row vector convention)
 */
static void update_combined_matrix(void) {
    if (g_gbi_state.combined_mtx_dirty) {
        /* N64 uses row vectors: v' = v * M
         * For combined transform: v'' = (v * MV) * P = v * (MV * P)
         * So combined = MV * P */
        mtx_multiply(g_gbi_state.combined_mtx,
                     g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr],
                     g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr]);
        g_gbi_state.combined_mtx_dirty = 0;

        /* Debug: print combined matrix - only for interesting projection matrices */
        static int cmtx_debug = 0;
        f32 proj_scale = fabsf(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][0]) +
                         fabsf(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][1][1]);
        f32 proj_row3_3 = g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][3];
        /* Only debug when projection matrix looks like real game matrix (not identity) */
        if (cmtx_debug < 10 && (fabsf(proj_row3_3) > 10.0f || proj_scale < 0.5f)) {
            printf("[COMB_REAL #%d] proj_scale=%.4f proj[3][3]=%.2f\n", cmtx_debug, proj_scale, proj_row3_3);
            printf("  PROJ: col2=(%.4f,%.4f,%.4f,%.4f) col3=(%.4f,%.4f,%.4f,%.4f)\n",
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][2],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][1][2],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][2][2],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][2],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][3],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][1][3],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][2][3],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][3]);
            printf("  MV row0=(%.2f,%.2f,%.2f,%.2f)\n",
                   g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr][0][0],
                   g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr][0][1],
                   g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr][0][2],
                   g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr][0][3]);
            printf("  COMB row3=(%.1f,%.1f,%.1f,%.1f) <- translation\n",
                   g_gbi_state.combined_mtx[3][0],
                   g_gbi_state.combined_mtx[3][1],
                   g_gbi_state.combined_mtx[3][2],
                   g_gbi_state.combined_mtx[3][3]);
            printf("  PROJ row3=(%.1f,%.1f,%.1f,%.1f) <- proj translation\n",
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][0],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][1],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][2],
                   g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][3]);
            fflush(stdout);
            cmtx_debug++;
        }
    }
}

/**
 * Initialize GBI interpreter
 */
void gbi_init(void) {
    memset(&g_gbi_state, 0, sizeof(g_gbi_state));

    g_gbi_state.geometry_mode = G_SHADE | G_SHADING_SMOOTH;
    g_gbi_state.texture_scale_s = 1.0f;
    g_gbi_state.texture_scale_t = 1.0f;

    /* Initialize identity matrices and stack pointers */
    g_gbi_state.projection_stack_ptr = 0;
    g_gbi_state.modelview_stack_ptr = 0;
    mtx_identity(g_gbi_state.projection_stack[0]);
    mtx_identity(g_gbi_state.modelview_stack[0]);
    g_gbi_state.combined_mtx_dirty = 1;

    /* Set default colors */
    g_gbi_state.prim_color[0] = 255;
    g_gbi_state.prim_color[1] = 255;
    g_gbi_state.prim_color[2] = 255;
    g_gbi_state.prim_color[3] = 255;

    /* Default viewport (640x480) */
    g_gbi_state.viewport_scale[0] = 320.0f;
    g_gbi_state.viewport_scale[1] = 240.0f;
    g_gbi_state.viewport_scale[2] = 511.0f;
    g_gbi_state.viewport_trans[0] = 320.0f;
    g_gbi_state.viewport_trans[1] = 240.0f;
    g_gbi_state.viewport_trans[2] = 511.0f;

    /* Initialize OpenGL viewport state */
    g_gbi_state.gl_viewport.x = 0;
    g_gbi_state.gl_viewport.y = 0;
    g_gbi_state.gl_viewport.width = 640;
    g_gbi_state.gl_viewport.height = 480;
    g_gbi_state.gl_viewport.valid = 0;
    glViewport(0, 0, 640, 480);

    /* Configure depth buffer to match N64 conventions:
     * N64 uses depth range 0 to 1.0 (fixed-point)
     * OpenGL default NDC is -1 to +1, but we can adjust with glDepthRange */
    glDepthRange(0.0, 1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    /* DIAGNOSTIC: Disable backface culling globally to ensure all triangles are visible */
    glDisable(GL_CULL_FACE);

    /* Register terrain texture dummy buffers for address lookup */
    extern void terrain_register_static_ptrs(void);
    terrain_register_static_ptrs();

    /* Initialize texture registry with terrain textures */
    texture_init_terrain();

    /* Test: Try loading binary terrain */
    extern int terrain_load_bin(const char* path);
    if (terrain_load_bin("assets/terrain/c1_1.bin") == 0) {
        printf("[GBI] Binary terrain loaded successfully!\n");
    } else {
        printf("[GBI] Binary terrain not found, using runtime-built terrain\n");
    }

    printf("GBI interpreter initialized\n");
}

/**
 * Shutdown GBI interpreter
 */
void gbi_shutdown(void) {
    printf("GBI stats: %d triangles, %d display lists, %d vertices, %d matrices\n",
           g_gbi_state.triangles_drawn,
           g_gbi_state.display_lists_executed,
           g_gbi_state.vertices_loaded,
           g_gbi_state.matrices_loaded);

    /* Cleanup texture registry */
    texture_shutdown();
}

/**
 * Identity Triangle Test - Phase 1 Diagnostic
 *
 * This function draws a hardcoded magenta triangle using pure OpenGL with
 * identity matrices. If this triangle appears centered on screen, it confirms:
 * 1. OpenGL context is working correctly
 * 2. Window buffer swap is working
 * 3. Basic rendering pipeline is functional
 *
 * The problem lies in GBI matrix/coordinate handling, not OpenGL itself.
 */
void gbi_test_identity_triangle(void) {
    /* Save current matrix mode */
    GLint prev_matrix_mode;
    glGetIntegerv(GL_MATRIX_MODE, &prev_matrix_mode);

    /* Set up identity transform pipeline */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Use full window viewport */
    glViewport(0, 0, 640, 480);

    /* Disable everything that could interfere */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);

    /* Draw magenta triangle in NDC space (-1 to +1)
     * Triangle should appear centered on screen if GL is working */
    glColor4f(1.0f, 0.0f, 1.0f, 1.0f);  /* Magenta */
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f, -0.5f, 0.0f);  /* Bottom-left */
    glVertex3f( 0.5f, -0.5f, 0.0f);  /* Bottom-right */
    glVertex3f( 0.0f,  0.5f, 0.0f);  /* Top-center */
    glEnd();

    /* Restore matrices */
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    /* Restore matrix mode */
    glMatrixMode(prev_matrix_mode);
}

/**
 * Debug: Log current matrix state for diagnostics
 * Call this after setting matrices to verify fixed-point conversion
 */
void gbi_log_matrix_state(const char* context) {
    static int log_count = 0;
    if (log_count >= 30) return;  /* Limit logging */

    f32 (*proj)[4] = g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr];
    f32 (*mv)[4] = g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr];

    printf("[MTX_LOG #%d] %s\n", log_count, context);
    printf("  PROJ diag: [%.6f, %.6f, %.6f, %.6f]\n",
           proj[0][0], proj[1][1], proj[2][2], proj[3][3]);
    printf("  PROJ row3: [%.2f, %.2f, %.2f, %.2f] (translation)\n",
           proj[3][0], proj[3][1], proj[3][2], proj[3][3]);

    /* Flag suspicious values */
    if (fabsf(proj[0][0]) > 1000000.0f || fabsf(proj[1][1]) > 1000000.0f) {
        printf("  !!! WARNING: Astronomical projection values - likely conversion error\n");
    }
    if (fabsf(proj[0][0]) < 0.00001f && fabsf(proj[1][1]) < 0.00001f) {
        printf("  !!! WARNING: Near-zero projection values - matrix may be identity\n");
    }

    /* Detect projection type by checking [2][3] */
    if (fabsf(proj[2][3]) > 0.5f) {
        printf("  Type: PERSPECTIVE (proj[2][3]=%.4f)\n", proj[2][3]);
    } else {
        printf("  Type: ORTHOGRAPHIC (proj[2][3]=%.4f)\n", proj[2][3]);
    }

    printf("  MV diag: [%.4f, %.4f, %.4f, %.4f]\n",
           mv[0][0], mv[1][1], mv[2][2], mv[3][3]);
    fflush(stdout);
    log_count++;
}

/**
 * Set framebuffer for rendering
 */
void gbi_set_framebuffer(void* fb, int width, int height) {
    g_gbi_state.framebuffer = fb;
    g_gbi_state.fb_width = width;
    g_gbi_state.fb_height = height;
}

/* ============================================================================
 * GBI Command Handlers
 * ============================================================================ */

/**
 * Execute G_MTX - Load matrix
 */
static int mtx_debug_count = 0;

static void gbi_cmd_mtx(u32 w0, u32 w1) {
    u32 params = w0 & 0x00FFFFFF;
    Mtx* mtx = (Mtx*)seg2ptr(w1);

    if (!mtx) return;

    f32 new_mtx[4][4];
    mtx_n64_to_float(mtx, new_mtx);

    /* Debug: Print the resolved matrix address and converted values */
    static int mtx_addr_debug = 0;
    if (mtx_addr_debug < 10) {
        printf("[MTX_ADDR] w1=0x%08X -> resolved=%p\n", w1, (void*)mtx);
        printf("[MTX_FLOAT] row0=(%.4f, %.4f, %.4f, %.4f)\n",
               new_mtx[0][0], new_mtx[0][1], new_mtx[0][2], new_mtx[0][3]);
        printf("[MTX_FLOAT] row1=(%.4f, %.4f, %.4f, %.4f)\n",
               new_mtx[1][0], new_mtx[1][1], new_mtx[1][2], new_mtx[1][3]);
        printf("[MTX_FLOAT] row2=(%.4f, %.4f, %.4f, %.4f)\n",
               new_mtx[2][0], new_mtx[2][1], new_mtx[2][2], new_mtx[2][3]);
        printf("[MTX_FLOAT] row3=(%.4f, %.4f, %.4f, %.4f)\n",
               new_mtx[3][0], new_mtx[3][1], new_mtx[3][2], new_mtx[3][3]);
        fflush(stdout);
        mtx_addr_debug++;
    }

    /* Validate matrix - only check for NaN/Inf which indicates bad pointer recovery.
     * Previous checks were too aggressive and flagged valid view matrices with large
     * translation values (e.g., camera at world coords 5000+). */
    int is_garbage = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (isnan(new_mtx[i][j]) || isinf(new_mtx[i][j])) {
                is_garbage = 1;
                break;
            }
        }
        if (is_garbage) break;
    }
    if (is_garbage) {
        static int garbage_warn = 0;
        if (garbage_warn < 3) {
            printf("[MTX] WARNING: NaN/Inf in matrix at 0x%08X, using identity\n", w1);
            fflush(stdout);
            garbage_warn++;
        }
        /* Set to identity - let the game logic handle camera setup */
        memset(new_mtx, 0, sizeof(new_mtx));
        new_mtx[0][0] = 1.0f;
        new_mtx[1][1] = 1.0f;
        new_mtx[2][2] = 1.0f;
        new_mtx[3][3] = 1.0f;
    }

    /* Note: gSPMatrix macro XORs params with G_MTX_PUSH (0x01), inverting bit 0.
     * So in the encoded command: bit 0 clear = PUSH, bit 0 set = NOPUSH.
     * Bit 1 is NOT inverted: G_MTX_LOAD = 0x02 means bit 1 set = load. */
    int push = (params & 0x01) == 0;       /* Push if bit 0 CLEAR (inverted by XOR) */
    int load = (params & 0x02) != 0;       /* Load if bit 1 SET (G_MTX_LOAD=0x02) */
    int projection = (params & 0x04) != 0; /* Projection (vs modelview) */

    /* Force LOAD mode when garbage matrix detected - replace entirely */
    if (is_garbage) load = 1;

    /* Debug logging for matrix loads */
    static int always_mtx_debug = 0;
    if (always_mtx_debug < 20 || mtx_debug_count < 15) {
        printf("[MTX #%d] w0=0x%08X params=0x%X proj=%d load=%d push=%d addr=0x%08X\n",
               always_mtx_debug, w0, params, projection, load, push, w1);
        always_mtx_debug++;
        for (int r = 0; r < 4; r++) {
            printf("  row%d=(%.2f,%.2f,%.2f,%.2f)\n", r,
                   new_mtx[r][0], new_mtx[r][1], new_mtx[r][2], new_mtx[r][3]);
        }
        fflush(stdout);
        mtx_debug_count++;
    }

    if (projection) {
        /* Handle projection matrix stack */
        if (push) {
            /* Push: Save the current matrix to stack[1] for later restoration.
             * The tile matrices will REPLACE the projection with an orthographic
             * transform for terrain rendering. */
            if (g_gbi_state.projection_stack_ptr < MTX_STACK_SIZE - 1) {
                g_gbi_state.projection_stack_ptr = 1;  /* Always push to slot 1 */
                memcpy(g_gbi_state.projection_stack[1],
                       g_gbi_state.projection_stack[0],
                       sizeof(g_gbi_state.projection_stack[0]));

                static int proj_push_debug = 0;
                if (proj_push_debug < 3) {
                    printf("[PROJ_PUSH] Saved stack[0] to stack[1], ptr=1\n");
                    proj_push_debug++;
                }
            }
            /* Note: load=1 will replace with tile matrix, which is correct
             * for orthographic terrain rendering. */
        } else {
            /* Non-push load: Reset stack to base level (stack[0]) */
            g_gbi_state.projection_stack_ptr = 0;
        }

        if (load) {
            memcpy(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr], new_mtx, sizeof(new_mtx));
            static int proj_load_debug = 0;
            if (proj_load_debug < 5) {
                printf("[PROJ_LOADED #%d] row0=(%.2f,%.2f,%.2f,%.2f) row3=(%.2f,%.2f,%.2f,%.2f)\n",
                       proj_load_debug,
                       g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][0], g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][1],
                       g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][2], g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][0][3],
                       g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][0], g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][1],
                       g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][2], g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr][3][3]);
                fflush(stdout);
                proj_load_debug++;
            }
        } else {
            f32 temp[4][4];
            memcpy(temp, g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr], sizeof(temp));
            /* N64 multiply mode: stack_top = stack_top * new_mtx */
            mtx_multiply(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr], temp, new_mtx);
        }
    } else {
        /* Modelview matrix */
        if (push && g_gbi_state.modelview_stack_ptr < MTX_STACK_SIZE - 1) {
            g_gbi_state.modelview_stack_ptr++;
            memcpy(g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr],
                   g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr - 1],
                   sizeof(g_gbi_state.modelview_stack[0]));
        }

        if (load) {
            memcpy(g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr],
                   new_mtx, sizeof(new_mtx));
        } else {
            f32 temp[4][4];
            memcpy(temp, g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr],
                   sizeof(temp));
            /* N64 multiply mode: stack_top = stack_top * new_mtx */
            mtx_multiply(g_gbi_state.modelview_stack[g_gbi_state.modelview_stack_ptr],
                        temp, new_mtx);
        }
    }

    g_gbi_state.combined_mtx_dirty = 1;
    g_gbi_state.matrices_loaded++;

    /* Shipwright approach: Do NOT load matrices to OpenGL.
     * We do software vertex transformation using combined_mtx in G_VTX.
     * OpenGL receives pre-transformed clip coordinates via glVertex4f.
     * GPU just does perspective divide and rasterization. */
}

/**
 * Execute G_POPMTX - Pop matrix from stack
 */
static void gbi_cmd_popmtx(u32 w0, u32 w1) {
    (void)w0;
    u32 count = w1 / 64;  /* Number of matrices to pop (each 64 bytes) */

    while (count > 0 && g_gbi_state.modelview_stack_ptr > 0) {
        g_gbi_state.modelview_stack_ptr--;
        count--;
    }

    g_gbi_state.combined_mtx_dirty = 1;
}

/**
 * Execute G_MV_VIEWPORT - Set viewport parameters
 * Converts N64 viewport to OpenGL glViewport call
 */
static int viewport_debug_count = 0;

static void gbi_cmd_set_viewport(Vp_t* vp) {
    if (!vp) return;

    /* Parse N64 viewport (values have 2-bit fraction, divide by 4)
     * Reference: Shipwright gfx_calc_and_set_viewport() */
    f32 scale_x = vp->vscale[0] / 4.0f;
    f32 scale_y = vp->vscale[1] / 4.0f;
    f32 trans_x = vp->vtrans[0] / 4.0f;
    f32 trans_y = vp->vtrans[1] / 4.0f;

    /* Calculate OpenGL viewport dimensions
     * N64 viewport: screen_x = ndc_x * scale_x + trans_x
     * For NDC [-1,1] -> screen [0, 2*scale_x]: width = 2*scale_x */
    f32 width = 2.0f * scale_x;
    f32 height = 2.0f * scale_y;
    f32 x = trans_x - scale_x;  /* Left edge */
    f32 y = trans_y - scale_y;  /* Top edge (N64 Y=0 at top) */

    /* Store viewport parameters */
    g_gbi_state.gl_viewport.x = x;
    g_gbi_state.gl_viewport.y = y;
    g_gbi_state.gl_viewport.width = width;
    g_gbi_state.gl_viewport.height = height;
    g_gbi_state.gl_viewport.valid = 1;

    /* Set OpenGL viewport (Y is flipped: OpenGL Y=0 at bottom)
     * Assuming 480-line display for Y flip calculation */
    int gl_y = (int)(480.0f - y - height);
    glViewport((int)x, gl_y, (int)width, (int)height);

    /* Debug logging */
    if (viewport_debug_count < 5) {
        printf("[VIEWPORT] N64: trans=(%.1f,%.1f) scale=(%.1f,%.1f) -> GL: (%.0f,%.0f,%.0f,%.0f)\n",
               trans_x, trans_y, scale_x, scale_y, x, (f32)gl_y, width, height);
        fflush(stdout);
        viewport_debug_count++;
    }
}

/**
 * Execute G_VTX - Load vertices into buffer
 * Transforms vertices from model space to clip space
 */
static int vtx_debug_count = 0;

static int vtx_cmd_count = 0;

static void gbi_cmd_vtx(u32 w0, u32 w1) {
    int n = ((w0 >> 12) & 0xFF);    /* Number of vertices */
    int vn = (w0 >> 1) & 0x7F;       /* v0 + n value */
    int v0 = vn - n;                 /* Starting vertex buffer index */
    Vtx* vtx = (Vtx*)seg2ptr(w1);    /* Vertex data pointer (translated) */

    /* Debug first few G_VTX commands */
    if (vtx_cmd_count < 10) {
        printf("[G_VTX #%d] w0=0x%08X w1=0x%08X -> n=%d v0=%d vtx=%p\n",
               vtx_cmd_count, w0, w1, n, v0, (void*)vtx);
        if (vtx) {
            printf("  vtx[0]: pos=(%d,%d,%d) color=(%d,%d,%d,%d)\n",
                   vtx[0].v.ob[0], vtx[0].v.ob[1], vtx[0].v.ob[2],
                   vtx[0].v.cn[0], vtx[0].v.cn[1], vtx[0].v.cn[2], vtx[0].v.cn[3]);
        }
        fflush(stdout);
        vtx_cmd_count++;
    }
    /* Debug binary terrain G_VTX (segment 8 addresses) */
    if ((w1 >> 24) == 0x08) {
        static int bin_vtx_debug = 0;
        if (bin_vtx_debug < 10) {
            printf("[SEG8_VTX] w1=0x%08X -> vtx=%p n=%d v0=%d seg8_base=0x%lX\n",
                   w1, (void*)vtx, n, v0,
                   (unsigned long)g_gbi_state.segments[8]);
            if (vtx) {
                printf("[SEG8_VTX] vtx[0]: pos=(%d,%d,%d) tc=(%d,%d) color=(%d,%d,%d,%d)\n",
                       vtx[0].v.ob[0], vtx[0].v.ob[1], vtx[0].v.ob[2],
                       vtx[0].v.tc[0], vtx[0].v.tc[1],
                       vtx[0].v.cn[0], vtx[0].v.cn[1], vtx[0].v.cn[2], vtx[0].v.cn[3]);
            } else {
                printf("[SEG8_VTX] ERROR: vtx is NULL! (seg8=0x%lX, offset=0x%06X)\n",
                       (unsigned long)g_gbi_state.segments[8], w1 & 0x00FFFFFF);
            }
            fflush(stdout);
            bin_vtx_debug++;
        }
    }
    /* Debug: log cube G_VTX specifically */
    if (g_in_cube_dl && g_cube_dl_debug < 60) {
        printf("[CUBE_VTX] n=%d v0=%d vtx=%p\n", n, v0, (void*)vtx);
        if (vtx) {
            printf("[CUBE_VTX] vtx[0]: pos=(%d,%d,%d) color=(%d,%d,%d,%d)\n",
                   vtx[0].v.ob[0], vtx[0].v.ob[1], vtx[0].v.ob[2],
                   vtx[0].v.cn[0], vtx[0].v.cn[1], vtx[0].v.cn[2], vtx[0].v.cn[3]);
        } else {
            printf("[CUBE_VTX] ERROR: vtx is NULL! w1=0x%08X\n", w1);
        }
        fflush(stdout);
    }

    GBI_DEBUG("[G_VTX] w1=0x%08X n=%d v0=%d vtx=%p\n", w1, n, v0, (void*)vtx);

    /* Early validation - check BEFORE any dereference */
    if (!vtx || n <= 0 || v0 < 0 || v0 + n > MAX_VERTICES) {
        GBI_DEBUG("[VTX] REJECTED early: vtx=%p n=%d v0=%d\n", (void*)vtx, n, v0);
        return;
    }

    /* Sanity check: verify pointer looks valid (not a small value from bad recovery) */
    if ((uintptr_t)vtx < 0x10000) {
        GBI_DEBUG("[VTX] REJECTED: bad pointer 0x%lX (too small)\n", (uintptr_t)vtx);
        return;
    }

    /* Copy raw vertices */
    memcpy(&g_gbi_state.vertex_buffer[v0], vtx, n * sizeof(Vtx));

    /* Update combined matrix: MP = modelview * projection
     * Shipwright approach: transform vertices on CPU, pass to GPU */
    update_combined_matrix();
    f32 (*mp)[4] = g_gbi_state.combined_mtx;

    /* Debug terrain vertex transform (segment 8 addresses) */
    static int terrain_transform_debug = 0;
    int is_terrain_vtx = ((w1 >> 24) == 0x08);

    /* Debug: confirm we enter the transform loop for terrain */
    if (is_terrain_vtx && terrain_transform_debug < 3) {
        printf("[VTX_LOOP_ENTER] terrain n=%d v0=%d vtx=%p\n", n, v0, (void*)vtx);
        fflush(stdout);
    }

    for (int i = 0; i < n; i++) {
        Vtx* src = &g_gbi_state.vertex_buffer[v0 + i];
        TransformedVertex* dst = &g_gbi_state.transformed[v0 + i];

        /* Get model-space position */
        f32 vx = (f32)src->v.ob[0];
        f32 vy = (f32)src->v.ob[1];
        f32 vz = (f32)src->v.ob[2];

        /* Transform by combined matrix: result = [vx,vy,vz,1] * MP
         * This produces clip coordinates - GPU will do perspective divide */
        f32 raw_x = vx * mp[0][0] + vy * mp[1][0] + vz * mp[2][0] + mp[3][0];
        f32 raw_y = vx * mp[0][1] + vy * mp[1][1] + vz * mp[2][1] + mp[3][1];
        f32 raw_z = vx * mp[0][2] + vy * mp[1][2] + vz * mp[2][2] + mp[3][2];
        f32 raw_w = vx * mp[0][3] + vy * mp[1][3] + vz * mp[2][3] + mp[3][3];

        /* Debug terrain transforms - first vertex of first 10 segment 8 G_VTX commands */
        if (is_terrain_vtx && i == 0) {
            if (terrain_transform_debug < 10) {
                printf("[TERRAIN_XFORM #%d] in=(%.0f,%.0f,%.0f) out=(%.1f,%.1f,%.1f,%.1f)\n",
                       terrain_transform_debug, vx, vy, vz, raw_x, raw_y, raw_z, raw_w);
                printf("[TERRAIN_XFORM #%d]   mp[0]=(%.4f,%.4f,%.4f,%.4f)\n",
                       terrain_transform_debug, mp[0][0], mp[0][1], mp[0][2], mp[0][3]);
                printf("[TERRAIN_XFORM #%d]   mp[3]=(%.1f,%.1f,%.1f,%.1f)\n",
                       terrain_transform_debug, mp[3][0], mp[3][1], mp[3][2], mp[3][3]);
                fflush(stdout);
            }
            terrain_transform_debug++;
        }

        /* Store clip coordinates DIRECTLY.
         * The combined matrix (modelview * projection) outputs clip coords.
         * Let the GPU do perspective divide: NDC = clip / w */
        dst->x = raw_x;
        dst->y = raw_y;
        dst->z = raw_z;
        dst->w = raw_w;

        /* Texture coordinates (fixed point 10.5 to float) */
        dst->s = (f32)src->v.tc[0] * g_gbi_state.texture_scale_s / 32.0f;
        dst->t = (f32)src->v.tc[1] * g_gbi_state.texture_scale_t / 32.0f;

        /* Copy vertex color/normal from source */
        dst->r = src->v.cn[0];
        dst->g = src->v.cn[1];
        dst->b = src->v.cn[2];
        dst->a = src->v.cn[3];
    }

    g_gbi_state.vertices_loaded += n;
}

/**
 * Helper: Draw a single vertex from the transformed buffer
 *
 * Shipwright approach: Submit clip coordinates directly to GPU.
 * The combined MVP matrix outputs clip coordinates.
 * GPU does perspective divide: NDC = clip / w
 * Y is flipped for OpenGL coordinate system (Y=0 at bottom vs top).
 */
static int draw_vtx_debug = 0;

/* Track current coordinate mode for draw_vertex Y-flip decision.
 * Set by setup_triangle_projection() before drawing triangles. */
static int g_current_ortho_mode = 0;

static void draw_vertex(int idx) {
    if (idx < 0 || idx >= MAX_VERTICES) return;

    TransformedVertex* v = &g_gbi_state.transformed[idx];

    /* Handle texture coordinates */
    if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
        f32 norm_s = v->s / (f32)g_gbi_state.gl_texture_width;
        f32 norm_t = v->t / (f32)g_gbi_state.gl_texture_height;
        glTexCoord2f(norm_s, norm_t);
    }

    /* Set vertex color */
    glColor4ub(v->r, v->g, v->b, v->a);

    /* Submit coordinates based on mode:
     * - Ortho mode: glOrtho already maps Y=0 at top, so pass Y as-is
     * - Perspective mode: negate Y for OpenGL (Y-up vs N64 Y-down)
     * The glOrtho(0, 640, 480, 0, -1, 1) call puts Y=0 at top, Y=480 at bottom. */
    if (g_current_ortho_mode) {
        /* Ortho: pass screen coords directly, glOrtho handles Y mapping */
        glVertex4f(v->x, v->y, v->z, v->w);
    } else {
        /* Perspective: negate Y for OpenGL coordinate system */
        glVertex4f(v->x, -v->y, v->z, v->w);
    }

    /* Debug logging for first few vertices */
    if (draw_vtx_debug < 20) {
        printf("[DRAW_VTX #%d] idx=%d clip=(%.2f,%.2f,%.2f,%.2f) color=(%d,%d,%d,%d)\n",
               draw_vtx_debug, idx, v->x, v->y, v->z, v->w,
               v->r, v->g, v->b, v->a);
        fflush(stdout);
        draw_vtx_debug++;
    }
}

/**
 * Detect if triangle vertices are in orthographic (screen) or perspective (clip) coords.
 * Returns 1 if ortho (all W ≈ 1), 0 if perspective (W varies).
 */
static int detect_ortho_mode(int v0, int v1, int v2) {
    TransformedVertex* tv0 = &g_gbi_state.transformed[v0];
    TransformedVertex* tv1 = &g_gbi_state.transformed[v1];
    TransformedVertex* tv2 = &g_gbi_state.transformed[v2];

    /* Check if W values are all approximately 1.0 (ortho) or varying (perspective) */
    f32 w_avg = (tv0->w + tv1->w + tv2->w) / 3.0f;
    f32 w_var = fabsf(tv0->w - w_avg) + fabsf(tv1->w - w_avg) + fabsf(tv2->w - w_avg);

    /* Ortho: W is constant ~1.0, screen coords typically 0-640/0-480 */
    /* Perspective: W varies based on depth, coords in clip space */
    int is_ortho = (fabsf(w_avg - 1.0f) < 0.01f && w_var < 0.01f);

    /* Additional check: ortho screen coords are typically > 1.0 */
    if (!is_ortho) {
        f32 max_coord = fabsf(tv0->x);
        if (fabsf(tv0->y) > max_coord) max_coord = fabsf(tv0->y);
        if (fabsf(tv1->x) > max_coord) max_coord = fabsf(tv1->x);
        if (fabsf(tv1->y) > max_coord) max_coord = fabsf(tv1->y);
        if (fabsf(tv2->x) > max_coord) max_coord = fabsf(tv2->x);
        if (fabsf(tv2->y) > max_coord) max_coord = fabsf(tv2->y);

        /* If coords are in screen range (> 10) and W ≈ 1, treat as ortho */
        if (max_coord > 10.0f && fabsf(w_avg - 1.0f) < 0.1f) {
            is_ortho = 1;
        }
    }

    return is_ortho;
}

/**
 * Set up OpenGL projection based on coordinate mode.
 * For ortho (UI/logos): map screen coords 0-640,0-480 to NDC
 * For perspective (3D): use identity, GPU does perspective divide
 */
static void setup_triangle_projection(int is_ortho) {
    g_current_ortho_mode = is_ortho;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (is_ortho) {
        /* Ortho mode: vertices are in screen coordinates (0-640, 0-480).
         * Map to NDC with Y=0 at top (N64 convention). */
        glOrtho(0, 640, 480, 0, -1, 1);
    }
    /* For perspective: identity matrix - vertices are clip coords,
     * GPU does perspective divide (NDC = clip / W) */

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Use stored viewport if valid, otherwise default */
    if (g_gbi_state.gl_viewport.valid) {
        int gl_y = (int)(480.0f - g_gbi_state.gl_viewport.y - g_gbi_state.gl_viewport.height);
        glViewport((int)g_gbi_state.gl_viewport.x, gl_y,
                   (int)g_gbi_state.gl_viewport.width, (int)g_gbi_state.gl_viewport.height);
    } else {
        glViewport(0, 0, 640, 480);
    }
}

/**
 * Execute G_TRI1 - Draw one triangle
 *
 * Detects ortho vs perspective mode and sets up projection accordingly.
 */
static int tri_debug_count = 0;

static void gbi_cmd_tri1(u32 w0, u32 w1) {
    (void)w0;

    int v0 = (w1 >> 16) & 0xFF;
    int v1 = (w1 >> 8) & 0xFF;
    int v2 = w1 & 0xFF;

    /* Vertex indices are multiplied by 2 in F3DEX2 */
    v0 /= 2;
    v1 /= 2;
    v2 /= 2;

    /* Detect coordinate mode based on W values */
    int is_ortho = detect_ortho_mode(v0, v1, v2);

    /* Debug logging for first few triangles */
    if (tri_debug_count < 20) {
        TransformedVertex* tv0 = &g_gbi_state.transformed[v0];
        TransformedVertex* tv1 = &g_gbi_state.transformed[v1];
        TransformedVertex* tv2 = &g_gbi_state.transformed[v2];
        printf("[TRI1] v=(%d,%d,%d) clip0=(%.2f,%.2f,%.2f,%.2f) ortho=%d\n",
               v0, v1, v2, tv0->x, tv0->y, tv0->z, tv0->w, is_ortho);
        fflush(stdout);
        tri_debug_count++;
    }

    /* Set up projection based on coordinate mode */
    setup_triangle_projection(is_ortho);

    /* Set up depth for N64 compatibility (0 to 1 range) */
    glDepthRange(0.0, 1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Set up OpenGL state */
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    /* Enable texturing if we have a valid texture */
    if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_gbi_state.gl_texture_id);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }

    /* Draw triangle */
    glBegin(GL_TRIANGLES);
    draw_vertex(v0);
    draw_vertex(v1);
    draw_vertex(v2);
    glEnd();

    g_gbi_state.triangles_drawn++;
}

/**
 * Execute G_TRI2 - Draw two triangles
 *
 * Detects ortho vs perspective mode and sets up projection accordingly.
 */
static void gbi_cmd_tri2(u32 w0, u32 w1) {
    /* First triangle */
    int v0 = (w0 >> 16) & 0xFF;
    int v1 = (w0 >> 8) & 0xFF;
    int v2 = w0 & 0xFF;

    /* Second triangle */
    int v3 = (w1 >> 16) & 0xFF;
    int v4 = (w1 >> 8) & 0xFF;
    int v5 = w1 & 0xFF;

    /* Vertex indices are multiplied by 2 in F3DEX2 */
    v0 /= 2; v1 /= 2; v2 /= 2;
    v3 /= 2; v4 /= 2; v5 /= 2;

    /* Detect coordinate mode based on first triangle's W values */
    int is_ortho = detect_ortho_mode(v0, v1, v2);

    /* Set up projection based on coordinate mode */
    setup_triangle_projection(is_ortho);

    /* Set up depth for N64 compatibility (0 to 1 range) */
    glDepthRange(0.0, 1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Set up OpenGL state */
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    /* Enable texturing if we have a valid texture */
    if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_gbi_state.gl_texture_id);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }

    /* Draw triangles */
    glBegin(GL_TRIANGLES);
    /* First triangle */
    draw_vertex(v0);
    draw_vertex(v1);
    draw_vertex(v2);
    /* Second triangle */
    draw_vertex(v3);
    draw_vertex(v4);
    draw_vertex(v5);
    glEnd();

    /* Debug: trace terrain triangles with segment 8 vertices */
    extern int g_frame_count;
    static int terrain_tri_debug = 0;
    if (g_frame_count >= 184 && g_frame_count <= 186 && terrain_tri_debug < 20) {
        TransformedVertex* tv0 = &g_gbi_state.transformed[v0];
        TransformedVertex* tv1 = &g_gbi_state.transformed[v1];
        printf("[TERRAIN_TRI F%d #%d] v=(%d,%d) clip0=(%.0f,%.0f,%.0f,%.1f) tex=%d\n",
               g_frame_count, terrain_tri_debug, v0, v1,
               tv0->x, tv0->y, tv0->z, tv0->w,
               g_gbi_state.gl_texture_valid ? g_gbi_state.gl_texture_id : -1);
        fflush(stdout);
        terrain_tri_debug++;
    }

    /* Debug: check for GL errors after terrain triangles at frame 185 */
    if (g_frame_count == 185) {
        static int tri2_gl_check = 0;
        if (tri2_gl_check < 5) {
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                printf("[TRI2_GL_ERROR F185 #%d] Error 0x%04X\n", tri2_gl_check, err);
                fflush(stdout);
            }
            tri2_gl_check++;
        }
    }

    g_gbi_state.triangles_drawn += 2;
}

/**
 * Execute gsSPNTriangles_5b (0xB1) - Draw up to 4 triangles with 5-bit packed indices
 *
 * This is a Dolphin/GC-specific extension used by Animal Crossing terrain.
 * Packs 4 triangles (12 indices) into 64 bits using 5-bit fields.
 *
 * Format (from gfxdis.py):
 *   tri0: bits 4-8, 9-13, 14-18 of w0
 *   tri1: bits 19-23, 24-28 of w0, bit 29+ spans into w1
 *   tri2: bits 2-6, 7-11, 12-16 of w1
 *   tri3: bits 17-21, 22-26, 27-31 of w1
 */
static void gbi_cmd_ntriangles_5b(u32 w0, u32 w1) {
    /* Extract 5-bit vertex indices for tri0 */
    int t0_v0 = (w0 >> 4) & 0x1F;
    int t0_v1 = (w0 >> 9) & 0x1F;
    int t0_v2 = (w0 >> 14) & 0x1F;

    /* Extract 5-bit vertex indices for tri1 (bit 29 spans words) */
    int t1_v0 = (w0 >> 19) & 0x1F;
    int t1_v1 = (w0 >> 24) & 0x1F;
    int t1_v2 = ((w0 >> 29) & 0x07) | ((w1 & 0x03) << 3);

    /* Extract 5-bit vertex indices for tri2 */
    int t2_v0 = (w1 >> 2) & 0x1F;
    int t2_v1 = (w1 >> 7) & 0x1F;
    int t2_v2 = (w1 >> 12) & 0x1F;

    /* Extract 5-bit vertex indices for tri3 */
    int t3_v0 = (w1 >> 17) & 0x1F;
    int t3_v1 = (w1 >> 22) & 0x1F;
    int t3_v2 = (w1 >> 27) & 0x1F;

    /* Debug logging for first few calls */
    static int ntri5b_debug = 0;
    if (ntri5b_debug < 10) {
        printf("[NTRI_5B] w0=0x%08X w1=0x%08X\n", w0, w1);
        printf("  tri0=(%d,%d,%d) tri1=(%d,%d,%d) tri2=(%d,%d,%d) tri3=(%d,%d,%d)\n",
               t0_v0, t0_v1, t0_v2, t1_v0, t1_v1, t1_v2,
               t2_v0, t2_v1, t2_v2, t3_v0, t3_v1, t3_v2);
        if (t0_v0 < 32 && t0_v1 < 32 && t0_v2 < 32) {
            TransformedVertex* tv = &g_gbi_state.transformed[t0_v0];
            printf("  t0_v0 pos=(%.1f,%.1f,%.1f)\n", tv->x, tv->y, tv->z);
        }
        fflush(stdout);
        ntri5b_debug++;
    }

    /* Skip degenerate triangles (all zeros) */
    int draw_t0 = !(t0_v0 == 0 && t0_v1 == 0 && t0_v2 == 0);
    int draw_t1 = !(t1_v0 == 0 && t1_v1 == 0 && t1_v2 == 0);
    int draw_t2 = !(t2_v0 == 0 && t2_v1 == 0 && t2_v2 == 0);
    int draw_t3 = !(t3_v0 == 0 && t3_v1 == 0 && t3_v2 == 0);

    /* Detect coordinate mode from first valid triangle */
    int is_ortho = 0;
    if (draw_t0) {
        is_ortho = detect_ortho_mode(t0_v0, t0_v1, t0_v2);
    } else if (draw_t1) {
        is_ortho = detect_ortho_mode(t1_v0, t1_v1, t1_v2);
    }

    /* Set up projection based on coordinate mode */
    setup_triangle_projection(is_ortho);

    /* Set up depth for N64 compatibility */
    glDepthRange(0.0, 1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Set up OpenGL state */
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    /* Enable texturing if we have a valid texture */
    if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_gbi_state.gl_texture_id);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }

    /* Draw triangles */
    glBegin(GL_TRIANGLES);

    if (draw_t0) {
        draw_vertex(t0_v0);
        draw_vertex(t0_v1);
        draw_vertex(t0_v2);
        g_gbi_state.triangles_drawn++;
    }
    if (draw_t1) {
        draw_vertex(t1_v0);
        draw_vertex(t1_v1);
        draw_vertex(t1_v2);
        g_gbi_state.triangles_drawn++;
    }
    if (draw_t2) {
        draw_vertex(t2_v0);
        draw_vertex(t2_v1);
        draw_vertex(t2_v2);
        g_gbi_state.triangles_drawn++;
    }
    if (draw_t3) {
        draw_vertex(t3_v0);
        draw_vertex(t3_v1);
        draw_vertex(t3_v2);
        g_gbi_state.triangles_drawn++;
    }

    glEnd();
}

/**
 * Execute G_TRIN_INDEPEND (0x0A) - Draw N independent triangles (Dolphin GBI)
 *
 * This is a Dolphin/GC-specific command that packs multiple triangles efficiently.
 * Uses 5-bit vertex indices (up to 32 vertices in buffer).
 *
 * Format (5-bit mode, bit 0 of w1 = 0):
 *   w0 bits 31-24: opcode (0x0A)
 *   w0 bits 23-17: triangle count - 1 (0 = 1 triangle, 1 = 2 triangles, etc.)
 *   w0 bits 16-2:  triangle 2 data (v6,v7,v8 packed)
 *   w0 bits 1-0:   upper 2 bits of triangle 1 data
 *
 *   w1 bits 31-19: lower 13 bits of triangle 1 data (v3,v4,v5 packed)
 *   w1 bits 18-4:  triangle 0 data (v0,v1,v2 packed)
 *   w1 bit 0:      mode flag (0 = 5-bit indices)
 *
 * Triangle data packing (15 bits per triangle):
 *   bits 14-10: v2 (5 bits)
 *   bits 9-5:   v1 (5 bits)
 *   bits 4-0:   v0 (5 bits)
 */
static void gbi_cmd_trin_independ(u32 w0, u32 w1) {
    int mode = w1 & 0x01;  /* 0 = 5-bit mode, 1 = 7-bit mode */
    int tri_count = ((w0 >> 17) & 0x7F) + 1;

    if (mode != 0) {
        /* 7-bit mode not implemented yet */
        GBI_DEBUG("[G_TRIN_INDEPEND] WARNING: 7-bit mode not implemented, skipping\n");
        return;
    }

    GBI_DEBUG("[G_TRIN_INDEPEND] w0=0x%08X w1=0x%08X tri_count=%d mode=%d\n",
               w0, w1, tri_count, mode);

    /* Extract triangle data for 5-bit mode */
    u32 tri0_data = (w1 >> 4) & 0x7FFF;   /* bits 18:4 of w1 */
    u32 tri1_data = ((w1 >> 19) & 0x1FFF) | ((w0 & 0x03) << 13);  /* bits 31:19 of w1 + bits 1:0 of w0 */
    u32 tri2_data = (w0 >> 2) & 0x7FFF;   /* bits 16:2 of w0 */

    /* Decode triangle 0: v0, v1, v2 */
    int v0 = tri0_data & 0x1F;
    int v1 = (tri0_data >> 5) & 0x1F;
    int v2 = (tri0_data >> 10) & 0x1F;

    /* Decode triangle 1: v3, v4, v5 */
    int v3 = tri1_data & 0x1F;
    int v4 = (tri1_data >> 5) & 0x1F;
    int v5 = (tri1_data >> 10) & 0x1F;

    /* Decode triangle 2: v6, v7, v8 */
    int v6 = tri2_data & 0x1F;
    int v7 = (tri2_data >> 5) & 0x1F;
    int v8 = (tri2_data >> 10) & 0x1F;

    GBI_DEBUG("[G_TRIN_INDEPEND] tri0=(%d,%d,%d) tri1=(%d,%d,%d) tri2=(%d,%d,%d)\n",
               v0, v1, v2, v3, v4, v5, v6, v7, v8);

    /* Detect coordinate mode from first triangle */
    int is_ortho = detect_ortho_mode(v0, v1, v2);

    /* Set up projection based on coordinate mode */
    setup_triangle_projection(is_ortho);

    /* Set up depth for N64 compatibility */
    glDepthRange(0.0, 1.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    /* Set up OpenGL state */
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    /* Enable texturing if we have a valid texture */
    if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_gbi_state.gl_texture_id);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        /* Use texture modulated by vertex color */
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  /* White base color */
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }

    /* Draw triangles */
    glBegin(GL_TRIANGLES);

    /* Triangle 0 */
    if (tri_count >= 1) {
        draw_vertex(v0);
        draw_vertex(v1);
        draw_vertex(v2);
    }

    /* Triangle 1 */
    if (tri_count >= 2) {
        draw_vertex(v3);
        draw_vertex(v4);
        draw_vertex(v5);
    }

    /* Triangle 2 */
    if (tri_count >= 3) {
        draw_vertex(v6);
        draw_vertex(v7);
        draw_vertex(v8);
    }

    glEnd();

    /* Debug marker disabled - game rendering working */

    g_gbi_state.triangles_drawn += tri_count;
}

/**
 * Execute G_TEXTURE - Set texture parameters
 */
static void gbi_cmd_texture(u32 w0, u32 w1) {
    int on_bits = ((w0 >> 1) & 0x7F);
    g_gbi_state.texture_on = on_bits != 0;
    g_gbi_state.texture_tile = (w0 >> 8) & 0x07;

    /* Scale values (16-bit fixed point, 16 = 1.0) */
    u16 scale_s = (w1 >> 16) & 0xFFFF;
    u16 scale_t = w1 & 0xFFFF;

    g_gbi_state.texture_scale_s = (f32)scale_s / 65536.0f;
    g_gbi_state.texture_scale_t = (f32)scale_t / 65536.0f;

    /* Debug: track texture state changes */
    static int tex_debug = 0;
    if (tex_debug < 10) {
        printf("[G_TEXTURE] on_bits=%d -> texture_on=%d tile=%d\n",
               on_bits, g_gbi_state.texture_on, g_gbi_state.texture_tile);
        fflush(stdout);
        tex_debug++;
    }
}

/* ============================================================================
 * Texture Conversion and Upload
 * ============================================================================ */

/**
 * Convert I4 (4-bit intensity) texture to RGBA8
 * GameCube I4 textures are stored in 8x8 tiles, 4 bits per pixel.
 * Each 8x8 tile is 32 bytes (8*8/2).
 * Output: Grayscale replicated to RGB, alpha = 255
 */
static void convert_I4_to_RGBA8(const u8* src, u8* dst, int width, int height) {
    /* GC I4 format: 8x8 tiles, each row of tile = 4 bytes (8 pixels * 0.5 bytes) */
    int tiles_x = (width + 7) / 8;
    int tiles_y = (height + 7) / 8;

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            /* Calculate tile offset in source data */
            int tile_idx = ty * tiles_x + tx;
            const u8* tile = src + tile_idx * 32;  /* 32 bytes per 8x8 I4 tile */

            /* Decode 8x8 tile */
            for (int row = 0; row < 8; row++) {
                int pixel_y = ty * 8 + row;
                if (pixel_y >= height) continue;

                for (int col = 0; col < 8; col += 2) {
                    int pixel_x = tx * 8 + col;
                    if (pixel_x >= width) continue;

                    /* Each byte = 2 pixels */
                    u8 byte = tile[row * 4 + col / 2];
                    u8 i0 = (byte >> 4) & 0x0F;  /* High nibble = left pixel */
                    u8 i1 = byte & 0x0F;          /* Low nibble = right pixel */

                    /* Expand 4-bit to 8-bit */
                    u8 c0 = (i0 << 4) | i0;
                    u8 c1 = (i1 << 4) | i1;

                    /* Write first pixel */
                    int dst_idx = (pixel_y * width + pixel_x) * 4;
                    dst[dst_idx + 0] = c0;  /* R */
                    dst[dst_idx + 1] = c0;  /* G */
                    dst[dst_idx + 2] = c0;  /* B */
                    dst[dst_idx + 3] = 255; /* A */

                    /* Write second pixel */
                    if (pixel_x + 1 < width) {
                        dst_idx += 4;
                        dst[dst_idx + 0] = c1;  /* R */
                        dst[dst_idx + 1] = c1;  /* G */
                        dst[dst_idx + 2] = c1;  /* B */
                        dst[dst_idx + 3] = 255; /* A */
                    }
                }
            }
        }
    }
}

/**
 * Convert I8 (8-bit intensity) texture to RGBA8
 */
static void convert_I8_to_RGBA8(const u8* src, u8* dst, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        u8 intensity = src[i];
        dst[i * 4 + 0] = intensity;  /* R */
        dst[i * 4 + 1] = intensity;  /* G */
        dst[i * 4 + 2] = intensity;  /* B */
        dst[i * 4 + 3] = 255;        /* A */
    }
}

/**
 * Convert IA4 (4-bit intensity + 4-bit alpha) texture to RGBA8
 * Each byte: high nibble = intensity, low nibble = alpha
 */
static void convert_IA4_to_RGBA8(const u8* src, u8* dst, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        u8 byte = src[i];
        u8 intensity = (byte >> 4) & 0x0F;
        u8 alpha = byte & 0x0F;

        /* Expand 4-bit to 8-bit */
        intensity = (intensity << 4) | intensity;
        alpha = (alpha << 4) | alpha;

        dst[i * 4 + 0] = intensity;  /* R */
        dst[i * 4 + 1] = intensity;  /* G */
        dst[i * 4 + 2] = intensity;  /* B */
        dst[i * 4 + 3] = alpha;      /* A */
    }
}

/**
 * Helper: Convert RGBA5551 palette entry to RGBA8
 */
static void palette_5551_to_rgba8(u16 entry, u8* r, u8* g, u8* b, u8* a) {
    u8 r5 = (entry >> 11) & 0x1F;
    u8 g5 = (entry >> 6) & 0x1F;
    u8 b5 = (entry >> 1) & 0x1F;
    u8 a1 = entry & 0x01;

    /* Expand 5-bit to 8-bit (replicate upper bits to fill lower bits) */
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g5 << 3) | (g5 >> 2);
    *b = (b5 << 3) | (b5 >> 2);
    *a = a1 ? 255 : 0;
}

/**
 * Convert CI4 (4-bit color index) texture to RGBA8
 * GameCube CI4 textures are stored in 8x8 tiles, 4 bits per pixel.
 * Each 8x8 tile is 32 bytes (8*8/2).
 * Palette is in TMEM at offset 2048, 16 colors in RGBA5551 format.
 */
static void convert_CI4_to_RGBA8(const u8* src, u8* dst, int width, int height, int pal) {
    /* Get palette from TMEM (each palette is 32 bytes = 16 colors * 2 bytes) */
    const u8* palette_ptr = &g_gbi_state.tmem[2048 + pal * 32];

    /* GC CI4 format: 8x8 tiles, each row of tile = 4 bytes (8 pixels * 0.5 bytes) */
    int tiles_x = (width + 7) / 8;
    int tiles_y = (height + 7) / 8;

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int tile_idx = ty * tiles_x + tx;
            const u8* tile = src + tile_idx * 32;  /* 32 bytes per 8x8 CI4 tile */

            for (int row = 0; row < 8; row++) {
                int pixel_y = ty * 8 + row;
                if (pixel_y >= height) continue;

                for (int col = 0; col < 8; col += 2) {
                    int pixel_x = tx * 8 + col;
                    if (pixel_x >= width) continue;

                    /* Each byte = 2 pixels */
                    u8 byte = tile[row * 4 + col / 2];
                    u8 idx0 = (byte >> 4) & 0x0F;  /* High nibble = left pixel */
                    u8 idx1 = byte & 0x0F;         /* Low nibble = right pixel */

                    /* Look up palette color (big-endian 16-bit RGBA5551) */
                    u16 color0 = (palette_ptr[idx0 * 2] << 8) | palette_ptr[idx0 * 2 + 1];
                    u16 color1 = (palette_ptr[idx1 * 2] << 8) | palette_ptr[idx1 * 2 + 1];

                    u8 r, g, b, a;

                    /* Write first pixel */
                    palette_5551_to_rgba8(color0, &r, &g, &b, &a);
                    int dst_idx = (pixel_y * width + pixel_x) * 4;
                    dst[dst_idx + 0] = r;
                    dst[dst_idx + 1] = g;
                    dst[dst_idx + 2] = b;
                    dst[dst_idx + 3] = a;

                    /* Write second pixel */
                    if (pixel_x + 1 < width) {
                        palette_5551_to_rgba8(color1, &r, &g, &b, &a);
                        dst_idx += 4;
                        dst[dst_idx + 0] = r;
                        dst[dst_idx + 1] = g;
                        dst[dst_idx + 2] = b;
                        dst[dst_idx + 3] = a;
                    }
                }
            }
        }
    }
}

/**
 * Convert CI8 (8-bit color index) texture to RGBA8
 * GameCube CI8 textures are stored in 8x4 tiles, 8 bits per pixel.
 * Each 8x4 tile is 32 bytes.
 * Palette is in TMEM at offset 2048, 256 colors in RGBA5551 format.
 */
static void convert_CI8_to_RGBA8(const u8* src, u8* dst, int width, int height, int pal) {
    /* Get palette from TMEM (256 colors * 2 bytes = 512 bytes max) */
    const u8* palette_ptr = &g_gbi_state.tmem[2048];
    (void)pal;  /* CI8 uses full 256-color palette starting at TMEM 2048 */

    /* GC CI8 format: 8x4 tiles, each tile = 32 bytes */
    int tiles_x = (width + 7) / 8;
    int tiles_y = (height + 3) / 4;

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int tile_idx = ty * tiles_x + tx;
            const u8* tile = src + tile_idx * 32;  /* 32 bytes per 8x4 CI8 tile */

            for (int row = 0; row < 4; row++) {
                int pixel_y = ty * 4 + row;
                if (pixel_y >= height) continue;

                for (int col = 0; col < 8; col++) {
                    int pixel_x = tx * 8 + col;
                    if (pixel_x >= width) continue;

                    /* Each byte = 1 pixel index */
                    u8 idx = tile[row * 8 + col];

                    /* Look up palette color (big-endian 16-bit RGBA5551) */
                    u16 color = (palette_ptr[idx * 2] << 8) | palette_ptr[idx * 2 + 1];

                    u8 r, g, b, a;
                    palette_5551_to_rgba8(color, &r, &g, &b, &a);

                    int dst_idx = (pixel_y * width + pixel_x) * 4;
                    dst[dst_idx + 0] = r;
                    dst[dst_idx + 1] = g;
                    dst[dst_idx + 2] = b;
                    dst[dst_idx + 3] = a;
                }
            }
        }
    }
}

/**
 * Convert RGBA16 (5551) texture to RGBA8
 * GameCube format: 4x4 tiles, 16 bits per pixel
 * Each tile is 32 bytes (4*4*2 bytes)
 * Pixel format: RRRRR GGGGG BBBBB A (5-5-5-1)
 */
static void convert_RGBA16_to_RGBA8(const u8* src, u8* dst, int width, int height) {
    int tiles_x = (width + 3) / 4;
    int tiles_y = (height + 3) / 4;

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int tile_idx = ty * tiles_x + tx;
            const u8* tile = src + tile_idx * 32;  /* 32 bytes per 4x4 tile */

            for (int row = 0; row < 4; row++) {
                int pixel_y = ty * 4 + row;
                if (pixel_y >= height) continue;

                for (int col = 0; col < 4; col++) {
                    int pixel_x = tx * 4 + col;
                    if (pixel_x >= width) continue;

                    /* Each pixel is 2 bytes, big-endian */
                    int byte_offset = (row * 4 + col) * 2;
                    u16 pixel = (tile[byte_offset] << 8) | tile[byte_offset + 1];

                    /* Extract 5551 components */
                    u8 r5 = (pixel >> 11) & 0x1F;
                    u8 g5 = (pixel >> 6) & 0x1F;
                    u8 b5 = (pixel >> 1) & 0x1F;
                    u8 a1 = pixel & 0x01;

                    /* Convert to 8-bit (replicate upper bits to fill lower bits) */
                    u8 r8 = (r5 << 3) | (r5 >> 2);
                    u8 g8 = (g5 << 3) | (g5 >> 2);
                    u8 b8 = (b5 << 3) | (b5 >> 2);
                    u8 a8 = a1 ? 255 : 0;

                    int dst_idx = (pixel_y * width + pixel_x) * 4;
                    dst[dst_idx + 0] = r8;
                    dst[dst_idx + 1] = g8;
                    dst[dst_idx + 2] = b8;
                    dst[dst_idx + 3] = a8;
                }
            }
        }
    }
}

/**
 * Convert IA8 (4-bit intensity + 4-bit alpha per pixel) texture to RGBA8
 * GameCube format: 4x4 tiles, 8 bits per pixel
 * Each tile is 16 bytes (4*4*1 bytes)
 * Pixel format: IIII AAAA (4-bit intensity, 4-bit alpha)
 */
static void convert_IA8_to_RGBA8(const u8* src, u8* dst, int width, int height) {
    int tiles_x = (width + 3) / 4;
    int tiles_y = (height + 3) / 4;

    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int tile_idx = ty * tiles_x + tx;
            const u8* tile = src + tile_idx * 16;  /* 16 bytes per 4x4 tile */

            for (int row = 0; row < 4; row++) {
                int pixel_y = ty * 4 + row;
                if (pixel_y >= height) continue;

                for (int col = 0; col < 4; col++) {
                    int pixel_x = tx * 4 + col;
                    if (pixel_x >= width) continue;

                    int byte_offset = row * 4 + col;
                    u8 byte = tile[byte_offset];

                    /* High nibble = intensity, low nibble = alpha */
                    u8 i4 = (byte >> 4) & 0x0F;
                    u8 a4 = byte & 0x0F;

                    /* Expand 4-bit to 8-bit */
                    u8 i8 = (i4 << 4) | i4;
                    u8 a8 = (a4 << 4) | a4;

                    int dst_idx = (pixel_y * width + pixel_x) * 4;
                    dst[dst_idx + 0] = i8;  /* R */
                    dst[dst_idx + 1] = i8;  /* G */
                    dst[dst_idx + 2] = i8;  /* B */
                    dst[dst_idx + 3] = a8;  /* A */
                }
            }
        }
    }
}

/**
 * Upload texture to OpenGL
 * Converts from N64/Dolphin format and creates GL texture
 */
static void upload_texture_to_gl(void) {
    if (g_gbi_state.timg_addr == 0 || g_gbi_state.timg_width <= 0) {
        return;
    }

    /* First check if this address matches a registered texture in the registry */
    int tex_idx = texture_find_by_addr(g_gbi_state.timg_addr);
    if (tex_idx >= 0) {
        /* Found a registered texture - use the registry to upload it */
        static int reg_tex_debug = 0;
        if (reg_tex_debug++ < 5) {
            printf("[TEXTURE] Using registered texture idx=%d for addr 0x%08X\n",
                   tex_idx, g_gbi_state.timg_addr);
            fflush(stdout);
        }

        /* Upload through the texture registry (handles palette expansion) */
        GLuint gl_tex = texture_upload_ci4_to_gl(tex_idx);
        if (gl_tex != 0) {
            g_gbi_state.gl_texture_id = gl_tex;
            g_gbi_state.gl_texture_valid = 1;
            g_gbi_state.gl_texture_width = texture_get_current_width();
            g_gbi_state.gl_texture_height = texture_get_current_height();
            return;
        }
    }

    int width = g_gbi_state.timg_width;
    int height = g_gbi_state.timg_height > 0 ? g_gbi_state.timg_height : width;
    int fmt = g_gbi_state.timg_fmt;
    int siz = g_gbi_state.timg_siz;

    /* Get source texture data */
    void* src = seg2ptr(g_gbi_state.timg_addr);
    if (src == NULL) {
        static int null_src_warn = 0;
        if (null_src_warn++ < 5) {
            printf("[TEXTURE] WARNING: NULL source pointer for addr 0x%08X\n",
                   g_gbi_state.timg_addr);
        }
        return;
    }

    /* Allocate RGBA8 buffer */
    u8* rgba_data = (u8*)malloc(width * height * 4);
    if (!rgba_data) {
        printf("[TEXTURE] ERROR: Failed to allocate RGBA buffer\n");
        return;
    }

    /* Convert based on format and size */
    int converted = 0;

    /* G_IM_FMT_RGBA = 0 */
    if (fmt == 0) {
        if (siz == 2) {  /* 16-bit (5551) */
            convert_RGBA16_to_RGBA8((const u8*)src, rgba_data, width, height);
            converted = 1;
        }
    }
    /* G_IM_FMT_CI = 2 (Color Index - paletted) */
    else if (fmt == 2) {
        /* Get palette index from the current texture tile */
        int tile = g_gbi_state.texture_tile;
        int pal = g_gbi_state.tiles[tile].pal;

        if (siz == 0) {  /* 4-bit indexed (CI4) - 16 colors */
            convert_CI4_to_RGBA8((const u8*)src, rgba_data, width, height, pal);
            converted = 1;
            static int ci4_debug = 0;
            if (ci4_debug++ < 3) {
                printf("[TEXTURE] CI4: %dx%d pal=%d tile=%d\n", width, height, pal, tile);
            }
        } else if (siz == 1) {  /* 8-bit indexed (CI8) - 256 colors */
            convert_CI8_to_RGBA8((const u8*)src, rgba_data, width, height, pal);
            converted = 1;
            static int ci8_debug = 0;
            if (ci8_debug++ < 3) {
                printf("[TEXTURE] CI8: %dx%d pal=%d tile=%d\n", width, height, pal, tile);
            }
        }
    }
    /* G_IM_FMT_IA = 3 (Intensity + Alpha) */
    else if (fmt == 3) {
        if (siz == 0) {  /* 4-bit (2-bit I + 2-bit A per pixel, packed) */
            convert_IA4_to_RGBA8((const u8*)src, rgba_data, width, height);
            converted = 1;
        } else if (siz == 1) {  /* 8-bit (4-bit I + 4-bit A) */
            convert_IA8_to_RGBA8((const u8*)src, rgba_data, width, height);
            converted = 1;
        }
    }
    /* G_IM_FMT_I = 4 (Intensity) */
    else if (fmt == 4) {
        if (siz == 0) {  /* 4-bit */
            convert_I4_to_RGBA8((const u8*)src, rgba_data, width, height);
            converted = 1;
        } else if (siz == 1) {  /* 8-bit */
            convert_I8_to_RGBA8((const u8*)src, rgba_data, width, height);
            converted = 1;
        }
    }

    if (!converted) {
        /* Fill with magenta to indicate unsupported format */
        for (int i = 0; i < width * height * 4; i += 4) {
            rgba_data[i + 0] = 255;  /* R */
            rgba_data[i + 1] = 0;    /* G */
            rgba_data[i + 2] = 255;  /* B */
            rgba_data[i + 3] = 255;  /* A */
        }
        static int unsupported_warn = 0;
        if (unsupported_warn++ < 5) {
            printf("[TEXTURE] WARNING: Unsupported format fmt=%d siz=%d, using magenta\n",
                   fmt, siz);
        }
    }

    /* Create/update OpenGL texture */
    if (g_gbi_state.gl_texture_id == 0) {
        glGenTextures(1, &g_gbi_state.gl_texture_id);
    }

    glBindTexture(GL_TEXTURE_2D, g_gbi_state.gl_texture_id);

    /* Set texture parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    /* Upload texture data */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);

    g_gbi_state.gl_texture_valid = 1;
    g_gbi_state.gl_texture_width = width;
    g_gbi_state.gl_texture_height = height;

    static int upload_debug = 0;
    if (upload_debug++ < 5) {
        printf("[TEXTURE] Uploaded %dx%d texture (fmt=%d siz=%d) to GL id=%u\n",
               width, height, fmt, siz, g_gbi_state.gl_texture_id);
        fflush(stdout);
    }

    free(rgba_data);
}

/**
 * Execute G_SETTIMG - Set texture image address
 * Opcode: 0xFD
 *
 * Format (N64): w0 = (0xFD << 24) | (fmt << 21) | (siz << 19) | (width-1)
 *               w1 = address of texture data
 *
 * Format (Dolphin): w0 = (0xFD << 24) | (fmt << 21) | (siz << 19) | (1 << 18) |
 *                        ((height/4-1) << 10) | (width-1)
 *                   w1 = address of texture data
 *
 * Dolphin variant has bit 18 set and includes height info.
 */
static void gbi_cmd_settimg(u32 w0, u32 w1) {
    int is_dolphin = (w0 >> 18) & 0x01;

    /* Debug: log every call */
    static int settimg_call_count = 0;
    if (settimg_call_count++ < 10) {
        printf("[G_SETTIMG] CALLED #%d: w0=0x%08X w1=0x%08X dolphin=%d\n",
               settimg_call_count, w0, w1, is_dolphin);
        fflush(stdout);
    }

    /* Special debug for terrain textures */
    if (w1 == 0x80200000 || w1 == 0x80200100) {
        printf("[G_SETTIMG] TERRAIN TEXTURE! addr=0x%08X\n", w1);
        fflush(stdout);
    }

    g_gbi_state.timg_fmt = (w0 >> 21) & 0x07;
    g_gbi_state.timg_siz = (w0 >> 19) & 0x03;
    g_gbi_state.timg_width = (w0 & 0x03FF) + 1;
    g_gbi_state.timg_addr = w1;
    g_gbi_state.timg_height = 0;  /* Default, overwritten for Dolphin */

    if (is_dolphin) {
        int height_div4_minus1 = (w0 >> 10) & 0xFF;
        g_gbi_state.timg_height = (height_div4_minus1 + 1) * 4;
        static int dolphin_tex_debug = 0;
        if (dolphin_tex_debug < 5) {
            printf("[G_SETTIMG_DOLPHIN] fmt=%d siz=%d w=%d h=%d addr=0x%08X\n",
                   g_gbi_state.timg_fmt, g_gbi_state.timg_siz,
                   g_gbi_state.timg_width, g_gbi_state.timg_height, w1);
            fflush(stdout);
            dolphin_tex_debug++;
        }
    }
}

/**
 * Execute G_SETTILE_DOLPHIN (0xD2) - Configure tile for Dolphin/GC
 *
 * This is a simplified tile setup used by Dolphin GBI.
 * Triggers texture upload to OpenGL.
 */
static void gbi_cmd_settile_dolphin(u32 w0, u32 w1) {
    (void)w1;
    static int dolphin_tile_debug = 0;

    int d_fmt = (w0 >> 20) & 0x0F;
    int tile = (w0 >> 16) & 0x07;
    int tlut_name = (w0 >> 12) & 0x0F;
    int wrap_s = (w0 >> 10) & 0x03;
    int wrap_t = (w0 >> 8) & 0x03;

    if (dolphin_tile_debug < 5) {
        printf("[G_SETTILE_DOLPHIN] d_fmt=%d tile=%d tlut=%d wrap_s=%d wrap_t=%d\n",
               d_fmt, tile, tlut_name, wrap_s, wrap_t);
        fflush(stdout);
        dolphin_tile_debug++;
    }

    /* Upload texture to OpenGL when tile is configured */
    if (g_gbi_state.timg_addr != 0) {
        upload_texture_to_gl();
        g_gbi_state.texture_on = 1;  /* Enable texturing for subsequent draws */
        g_gbi_state.texture_tile = tile;
    }
}

/**
 * Execute G_SETTILE - Configure a tile descriptor
 * Opcode: 0xF5
 *
 * Format: w0 = (0xF5 << 24) | (fmt << 21) | (siz << 19) | (line << 9) | (tmem)
 *         w1 = (tile << 24) | (pal << 20) | (cmt << 18) | (maskt << 14) |
 *              (shiftt << 10) | (cms << 8) | (masks << 4) | shifts
 *
 * This configures how the RDP interprets texture data for a given tile.
 */
static void gbi_cmd_settile(u32 w0, u32 w1) {
    int tile = (w1 >> 24) & 0x07;

    g_gbi_state.tiles[tile].fmt   = (w0 >> 21) & 0x07;
    g_gbi_state.tiles[tile].siz   = (w0 >> 19) & 0x03;
    g_gbi_state.tiles[tile].line  = (w0 >> 9) & 0x1FF;
    g_gbi_state.tiles[tile].tmem  = w0 & 0x1FF;
    g_gbi_state.tiles[tile].pal   = (w1 >> 20) & 0x0F;
    g_gbi_state.tiles[tile].clampt = (w1 >> 19) & 0x01;
    g_gbi_state.tiles[tile].mirrort = (w1 >> 18) & 0x01;
    g_gbi_state.tiles[tile].maskt = (w1 >> 14) & 0x0F;
    g_gbi_state.tiles[tile].shiftt = (w1 >> 10) & 0x0F;
    g_gbi_state.tiles[tile].clamps = (w1 >> 9) & 0x01;
    g_gbi_state.tiles[tile].mirrors = (w1 >> 8) & 0x01;
    g_gbi_state.tiles[tile].masks = (w1 >> 4) & 0x0F;
    g_gbi_state.tiles[tile].shifts = w1 & 0x0F;
}

/**
 * Execute G_LOADBLOCK - Load texture data to TMEM
 * Opcode: 0xF3
 *
 * Format: w0 = (0xF3 << 24) | (sl << 12) | tl
 *         w1 = (tile << 24) | (sh << 12) | dxt
 *
 * Copies texture data from DRAM (at address set by G_SETTIMG) into TMEM.
 * sl, tl = start coordinates (always 0,0 for LOADBLOCK)
 * sh = number of texels to load - 1
 * dxt = delta T per scanline (for mipmaps, usually 0)
 */
static void gbi_cmd_loadblock(u32 w0, u32 w1) {
    int tile = (w1 >> 24) & 0x07;
    int sl = (w0 >> 12) & 0xFFF;  /* Always 0 for LOADBLOCK */
    int tl = w0 & 0xFFF;          /* Always 0 for LOADBLOCK */
    int sh = (w1 >> 12) & 0xFFF;  /* Number of texels - 1 */
    int dxt = w1 & 0xFFF;         /* Delta T */

    (void)sl; (void)tl; (void)dxt;  /* Unused in basic implementation */

    /* Calculate bytes to load based on texture size */
    int num_texels = sh + 1;
    int bytes_per_texel;

    switch (g_gbi_state.timg_siz) {
        case 0: bytes_per_texel = 1; num_texels /= 2; break;  /* 4-bit: 0.5 bytes */
        case 1: bytes_per_texel = 1; break;   /* 8-bit */
        case 2: bytes_per_texel = 2; break;   /* 16-bit */
        case 3: bytes_per_texel = 4; break;   /* 32-bit */
        default: bytes_per_texel = 2; break;
    }

    int bytes_to_load = num_texels * bytes_per_texel;
    if (bytes_to_load > 4096) bytes_to_load = 4096;  /* TMEM is 4KB */

    /* Copy texture data to TMEM */
    int tmem_offset = g_gbi_state.tiles[tile].tmem * 8;  /* TMEM addr is in 64-bit words */
    if (tmem_offset + bytes_to_load > 4096) {
        bytes_to_load = 4096 - tmem_offset;
    }

    if (bytes_to_load > 0 && g_gbi_state.timg_addr != 0) {
        void* src = seg2ptr(g_gbi_state.timg_addr);
        if (src != NULL) {
            memcpy(&g_gbi_state.tmem[tmem_offset], src, bytes_to_load);
            g_gbi_state.tmem_loaded = 1;
        }
    }
}

/**
 * Execute G_SETTILESIZE - Set tile size/coordinates
 * Opcode: 0xF2
 *
 * Format: w0 = (0xF2 << 24) | (sl << 12) | tl
 *         w1 = (tile << 24) | (sh << 12) | th
 *
 * Sets the texture coordinates for the tile (10.2 fixed point).
 */
static void gbi_cmd_settilesize(u32 w0, u32 w1) {
    int tile = (w1 >> 24) & 0x07;

    g_gbi_state.tiles[tile].sl = (w0 >> 12) & 0xFFF;
    g_gbi_state.tiles[tile].tl = w0 & 0xFFF;
    g_gbi_state.tiles[tile].sh = (w1 >> 12) & 0xFFF;
    g_gbi_state.tiles[tile].th = w1 & 0xFFF;
}

/**
 * Execute G_LOADTILE - Load a rectangular region of texture
 * Opcode: 0xF4
 *
 * Similar to G_LOADBLOCK but uses tile coordinates.
 */
static void gbi_cmd_loadtile(u32 w0, u32 w1) {
    int tile = (w1 >> 24) & 0x07;
    int sl = (w0 >> 12) & 0xFFF;
    int tl = w0 & 0xFFF;
    int sh = (w1 >> 12) & 0xFFF;
    int th = w1 & 0xFFF;

    /* Convert from 10.2 fixed point */
    int left = sl >> 2;
    int top = tl >> 2;
    int right = sh >> 2;
    int bottom = th >> 2;

    /* Calculate size */
    int width = right - left + 1;
    int height = bottom - top + 1;

    /* Calculate bytes per row and total */
    int bytes_per_texel;
    switch (g_gbi_state.timg_siz) {
        case 0: bytes_per_texel = 1; width /= 2; break;  /* 4-bit */
        case 1: bytes_per_texel = 1; break;   /* 8-bit */
        case 2: bytes_per_texel = 2; break;   /* 16-bit */
        case 3: bytes_per_texel = 4; break;   /* 32-bit */
        default: bytes_per_texel = 2; break;
    }

    int bytes_to_load = width * height * bytes_per_texel;
    if (bytes_to_load > 4096) bytes_to_load = 4096;

    int tmem_offset = g_gbi_state.tiles[tile].tmem * 8;
    if (tmem_offset + bytes_to_load > 4096) {
        bytes_to_load = 4096 - tmem_offset;
    }

    if (bytes_to_load > 0 && g_gbi_state.timg_addr != 0) {
        void* src = seg2ptr(g_gbi_state.timg_addr);
        if (src != NULL) {
            memcpy(&g_gbi_state.tmem[tmem_offset], src, bytes_to_load);
            g_gbi_state.tmem_loaded = 1;
        }
    }
}

/**
 * Execute G_LOADTLUT - Load texture lookup table (palette)
 * Opcode: 0xF0
 *
 * Format: w0 = (0xF0 << 24)
 *         w1 = (tile << 24) | (count << 14)
 *
 * Loads a color palette for CI (Color Index) textures.
 */
static void gbi_cmd_loadtlut(u32 w0, u32 w1) {
    (void)w0;
    int tile = (w1 >> 24) & 0x07;
    int count = ((w1 >> 14) & 0x3FF) + 1;  /* Number of palette entries - 1 */

    /* Palettes are loaded to upper half of TMEM (at 0x800 = 2048 bytes) */
    int pal_offset = g_gbi_state.tiles[tile].pal;
    int tmem_offset = 2048 + (pal_offset * 256);  /* Each palette is 256 bytes (16 colors * 16 bytes) */

    int bytes_to_load = count * 2;  /* Each palette entry is 16-bit RGBA5551 */
    if (tmem_offset + bytes_to_load > 4096) {
        bytes_to_load = 4096 - tmem_offset;
    }

    if (bytes_to_load > 0 && g_gbi_state.timg_addr != 0) {
        void* src = seg2ptr(g_gbi_state.timg_addr);
        if (src != NULL) {
            memcpy(&g_gbi_state.tmem[tmem_offset], src, bytes_to_load);
        }
    }
}

/* ============================================================================
 * Dolphin-specific GBI Commands
 * These are GameCube extensions used by Animal Crossing
 * ============================================================================ */

/* External: texture registry functions */
extern int texture_find_by_seg9_offset(u32 offset);
extern int texture_find_palette_by_seg9_offset(u32 offset);
extern void texture_set_active_tlut(int palette_idx);
extern GLuint texture_upload_ci4_to_gl(int tex_idx);

/**
 * Execute G_LOADTLUT_DOLPHIN (0xDD) - Load palette (Dolphin format)
 *
 * Format from terrain_to_bin.py:
 *   w0 = (G_LOADTLUT_DOLPHIN << 24) | (tile << 16) | count
 *   w1 = segmented address (segment 9 offset)
 *
 * This loads a TLUT (palette) for CI textures using the texture registry.
 */
static void gbi_cmd_loadtlut_dolphin(u32 w0, u32 w1) {
    int tile = (w0 >> 16) & 0xFF;
    int count = w0 & 0xFFFF;
    u32 seg_addr = w1;

    /* Extract segment 9 offset from segmented address */
    u32 offset = seg_addr & 0x00FFFFFF;

    static int tlut_dolphin_debug = 0;
    if (tlut_dolphin_debug < 5) {
        printf("[G_LOADTLUT_DOLPHIN] tile=%d count=%d seg_addr=0x%08X offset=0x%X\n",
               tile, count, seg_addr, offset);
        fflush(stdout);
        tlut_dolphin_debug++;
    }

    /* Look up palette by segment 9 offset */
    int pal_idx = texture_find_palette_by_seg9_offset(offset);
    if (pal_idx >= 0) {
        texture_set_active_tlut(pal_idx);
        if (tlut_dolphin_debug < 10) {
            printf("[G_LOADTLUT_DOLPHIN] Found palette at index %d\n", pal_idx);
        }
    } else {
        if (tlut_dolphin_debug < 10) {
            printf("[G_LOADTLUT_DOLPHIN] WARNING: Palette not found for offset 0x%X\n", offset);
        }
    }
}

/**
 * Execute G_LOADTEXBLOCK_DOLPHIN (0xD0) - Load CI4 texture (Dolphin format)
 *
 * Format from terrain_to_bin.py:
 *   w0 = (G_LOADTEXBLOCK_DOLPHIN << 24) | (fmt << 21) | (siz << 19) | ((width-1) << 8) | (height-1)
 *   w1 = segmented address (segment 9 offset)
 *
 * This loads a texture block for CI4/CI8 textures using the texture registry.
 */
static void gbi_cmd_loadtexblock_dolphin(u32 w0, u32 w1) {
    int fmt = (w0 >> 21) & 0x07;
    int siz = (w0 >> 19) & 0x03;
    int width = ((w0 >> 8) & 0xFF) + 1;
    int height = (w0 & 0xFF) + 1;
    u32 seg_addr = w1;

    /* Extract segment 9 offset from segmented address */
    u32 offset = seg_addr & 0x00FFFFFF;

    static int texblock_dolphin_debug = 0;
    if (texblock_dolphin_debug < 30) {
        printf("[G_LOADTEXBLOCK_DOLPHIN] fmt=%d siz=%d %dx%d seg_addr=0x%08X offset=0x%X\n",
               fmt, siz, width, height, seg_addr, offset);
        fflush(stdout);
        texblock_dolphin_debug++;
    }

    /* Look up texture by segment 9 offset */
    int tex_idx = texture_find_by_seg9_offset(offset);
    if (tex_idx >= 0) {
        /* Upload texture to OpenGL */
        GLuint gl_tex = texture_upload_ci4_to_gl(tex_idx);
        if (gl_tex != 0) {
            glBindTexture(GL_TEXTURE_2D, gl_tex);
            g_gbi_state.texture_on = 1;
            g_gbi_state.gl_texture_id = gl_tex;    /* FIX: Update texture ID */
            g_gbi_state.gl_texture_valid = 1;      /* FIX: Mark texture as valid */
            g_gbi_state.gl_texture_width = width;
            g_gbi_state.gl_texture_height = height;
            if (texblock_dolphin_debug < 10) {
                printf("[G_LOADTEXBLOCK_DOLPHIN] Bound texture GL id=%u\n", gl_tex);
            }
        }
    } else {
        if (texblock_dolphin_debug < 10) {
            printf("[G_LOADTEXBLOCK_DOLPHIN] WARNING: Texture not found for offset 0x%X\n", offset);
        }
    }
}

/**
 * Execute G_SETOTHERMODE_L - Set RDP other mode (low bits)
 */
static void gbi_cmd_setothermode_l(u32 w0, u32 w1) {
    int shift = (w0 >> 8) & 0xFF;
    int len = (w0 & 0xFF) + 1;
    u32 mask = ((1U << len) - 1) << shift;

    g_gbi_state.othermode_low = (g_gbi_state.othermode_low & ~mask) | (w1 & mask);
}

/**
 * Execute G_SETOTHERMODE_H - Set RDP other mode (high bits)
 */
static void gbi_cmd_setothermode_h(u32 w0, u32 w1) {
    int shift = (w0 >> 8) & 0xFF;
    int len = (w0 & 0xFF) + 1;
    u32 mask = ((1U << len) - 1) << shift;

    g_gbi_state.othermode_high = (g_gbi_state.othermode_high & ~mask) | (w1 & mask);
}

/**
 * Execute G_SETPRIMCOLOR
 */
static void gbi_cmd_setprimcolor(u32 w0, u32 w1) {
    (void)w0;
    g_gbi_state.prim_color[0] = (w1 >> 24) & 0xFF;
    g_gbi_state.prim_color[1] = (w1 >> 16) & 0xFF;
    g_gbi_state.prim_color[2] = (w1 >> 8) & 0xFF;
    g_gbi_state.prim_color[3] = w1 & 0xFF;

    static int prim_debug = 0;
    if (prim_debug++ < 5) {
        printf("[G_SETPRIMCOLOR] RGBA=(%d,%d,%d,%d)\n",
               g_gbi_state.prim_color[0], g_gbi_state.prim_color[1],
               g_gbi_state.prim_color[2], g_gbi_state.prim_color[3]);
    }
}

/**
 * Execute G_SETENVCOLOR
 */
static void gbi_cmd_setenvcolor(u32 w0, u32 w1) {
    (void)w0;
    g_gbi_state.env_color[0] = (w1 >> 24) & 0xFF;
    g_gbi_state.env_color[1] = (w1 >> 16) & 0xFF;
    g_gbi_state.env_color[2] = (w1 >> 8) & 0xFF;
    g_gbi_state.env_color[3] = w1 & 0xFF;

    static int env_debug = 0;
    if (env_debug++ < 10) {
        printf("[G_SETENVCOLOR] RGBA=(%d,%d,%d,%d)\n",
               g_gbi_state.env_color[0], g_gbi_state.env_color[1],
               g_gbi_state.env_color[2], g_gbi_state.env_color[3]);
    }
}

/**
 * Execute G_GEOMETRYMODE
 */
static void gbi_cmd_geometrymode(u32 w0, u32 w1) {
    u32 clear_bits = ~(w0 & 0x00FFFFFF);
    u32 set_bits = w1;

    g_gbi_state.geometry_mode &= clear_bits;
    g_gbi_state.geometry_mode |= set_bits;

    /* Update OpenGL state based on geometry mode */
    if (g_gbi_state.geometry_mode & G_ZBUFFER) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (g_gbi_state.geometry_mode & G_CULL_BACK) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else if (g_gbi_state.geometry_mode & G_CULL_FRONT) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

/**
 * Execute G_MOVEWORD - Configure RDP/RSP state
 * Old GBI format: opcode 0xBC
 * F3DEX2 format: opcode 0xDB
 *
 * Format: w0 = (opcode << 24) | (index << 16) | offset
 *         w1 = data value
 */
static int moveword_debug = 0;

static void gbi_cmd_moveword(u32 w0, u32 w1) {
    int index = (w0 >> 16) & 0xFF;
    int offset = w0 & 0xFFFF;

    if (moveword_debug < 20) {
        printf("[MOVEWORD] index=0x%02X offset=0x%04X w1=0x%08X\n", index, offset, w1);
        fflush(stdout);
        moveword_debug++;
    }

    switch (index) {
        case G_MW_SEGMENT:
            /* Set segment register: offset/4 = segment number */
            {
                int seg = offset / 4;
                if (seg >= 0 && seg < MAX_SEGMENTS) {
                    /* Recover full 64-bit address from truncated 32-bit value.
                     * GBI macros truncate pointers: (u32)(uintptr_t)(addr)
                     * We need to combine with heap base to get full address.
                     *
                     * NOTE: On 64-bit PC, truncated heap pointers can have ANY pattern
                     * in the lower 32 bits, including 0x8xxxxxxx which looks like N64
                     * physical addresses. We MUST try heap recovery first. */
                    if (w1 == 0) {
                        /* Zero address - store 0 */
                        g_gbi_state.segments[seg] = 0;
                    } else {
                        /* Try to recover full 64-bit pointer */
                        void* recovered = NULL;

                        /* For segment 10 (terrain), check test terrain pointer */
                        if (seg == 10) {
                            extern void* test_terrain_get_fullptr(u32 truncated_addr);
                            recovered = test_terrain_get_fullptr(w1);
                        }

                        if (recovered != NULL) {
                            /* Use exact pointer from test terrain */
                            g_gbi_state.segments[seg] = (uintptr_t)recovered;
                        } else if (g_heap_base != 0) {
                            /* Truncated PC pointer - recover full 64-bit address */
                            g_gbi_state.segments[seg] = g_heap_base | (uintptr_t)w1;
                        } else {
                            g_gbi_state.segments[seg] = (uintptr_t)w1;
                        }
                    }

                    /* Debug: Log segment 10 (terrain) being set */
                    if (seg == 10) {
                        static int seg10_debug = 0;
                        if (seg10_debug < 5) {
                            printf("[GBI] Segment 10 set: w1=0x%08X -> %p\n",
                                   w1, (void*)g_gbi_state.segments[seg]);
                            fflush(stdout);
                            seg10_debug++;
                        }
                    }
                }
            }
            break;

        case G_MW_NUMLIGHT:
            /* Set number of lights */
            g_gbi_state.num_lights = (w1 / 24) - 1;
            if (g_gbi_state.num_lights < 0) g_gbi_state.num_lights = 0;
            if (g_gbi_state.num_lights > 7) g_gbi_state.num_lights = 7;
            break;

        case G_MW_FOG:
            /* Set fog parameters */
            /* w1 = (multiplier << 16) | offset */
            /* For now, just store the values; apply during rendering */
            break;

        case G_MW_CLIP:
            /* Set clip ratio - used for guard band clipping */
            /* Not critical for basic rendering */
            break;

        case G_MW_PERSPNORM:
            /* Set perspective normalization value */
            /* Used for Z-buffer precision; OpenGL handles this */
            break;

        case G_MW_MATRIX:
            /* Modify matrix element (rarely used) */
            break;

        case G_MW_FORCEMTX:
            /* Force matrix update */
            g_gbi_state.combined_mtx_dirty = 1;
            break;

        default:
            /* Unknown moveword index */
            break;
    }
}

/**
 * Execute G_SETSCISSOR - Set clipping rectangle
 * Opcode: 0xED
 *
 * Format: w0 = (0xED << 24) | (ulx*4 << 12) | (uly*4)
 *         w1 = (mode << 24) | (lrx*4 << 12) | (lry*4)
 */
static void gbi_cmd_setscissor(u32 w0, u32 w1) {
    /* Extract coordinates (in 10.2 fixed point, so divide by 4) */
    int ulx = ((w0 >> 12) & 0xFFF) / 4;
    int uly = (w0 & 0xFFF) / 4;
    int lrx = ((w1 >> 12) & 0xFFF) / 4;
    int lry = (w1 & 0xFFF) / 4;

    /* Mode is in bits 24-25 of w1 (interlace mode, usually 0) */
    /* int mode = (w1 >> 24) & 0x03; */

    /* Calculate width and height */
    int width = lrx - ulx;
    int height = lry - uly;

    if (width > 0 && height > 0) {
        /* OpenGL scissor origin is bottom-left, N64 is top-left */
        /* Assuming 480-line display for Y flip */
        int gl_y = 480 - lry;
        if (gl_y < 0) gl_y = 0;

        glEnable(GL_SCISSOR_TEST);
        glScissor(ulx, gl_y, width, height);
    }
}

/**
 * Execute a single display list command
 */
static int g_gbi_debug_frame = 0;
static int opcode_count[256] = {0};  /* Track opcode usage */
static int opcode_summary_printed = 0;
static u32 gbi_last_gdl_w1 = 0;  /* Track last G_DL segmented address for history */

static int gbi_execute_cmd(Gfx* cmd) {
    /* Validate command pointer before dereferencing */
    uintptr_t cmd_ptr = (uintptr_t)cmd;
    if (cmd_ptr < 0x10000 || cmd_ptr > 0x7fffffffffff) {
        static int bad_cmd_warn = 0;
        if (bad_cmd_warn < 5) {
            printf("[GBI_CMD BAD PTR] cmd=%p TERMINATING\n", (void*)cmd);
            fflush(stdout);
            bad_cmd_warn++;
        }
        return 0;  /* End this display list */
    }

    /* Access command words through union - handles both raw u64 and struct access */
    u32 w0 = cmd->words.w0;
    u32 w1 = cmd->words.w1;
    u8 opcode = (w0 >> 24) & 0xFF;

    extern int g_frame_count;  /* Needed for various debug checks */

    /* Debug: disabled for now */
#if 0
    static int f_cmd_count = 0;
    if (g_frame_count >= 184 && g_frame_count <= 186) {
        f_cmd_count++;
        if (f_cmd_count < 100 || (f_cmd_count % 500 == 0)) {
            printf("[F%d_CMD #%d] cmd=%p op=0x%02X w0=0x%08X w1=0x%08X\n",
                   g_frame_count, f_cmd_count, (void*)cmd, opcode, w0, w1);
            fflush(stdout);
        }
    }
#endif

    /* Track opcode usage */
    opcode_count[opcode]++;

    /* Debug: log all commands in the cube's display list */
    if (g_in_cube_dl && g_cube_dl_debug < 50) {
        printf("[CUBE_CMD] op=0x%02X w0=0x%08X w1=0x%08X\n", opcode, w0, w1);
        fflush(stdout);
        g_cube_dl_debug++;
    }

    /* Treat completely empty command (all zeros) as implicit end.
     * This provides resilience against uninitialized display list buffers,
     * which would otherwise cause infinite loops when following branch chains. */
    if (w0 == 0 && w1 == 0) {
        /* Debug: Log when we hit a zero command - this may indicate
         * uninitialized gaps in the display list buffer */
        static int zero_cmd_warn = 0;
        if (zero_cmd_warn < 30) {
            printf("[GBI ZERO] Hit NULL command at %p (terminating this DL)\n",
                   (void*)cmd);
            fflush(stdout);
            zero_cmd_warn++;
        }
        return 0;  /* End display list */
    }

    /* Detect garbage display list data from bad pointer recovery.
     * Pattern: constant w1=0x00007FD4 (or similar small values) with varying w0.
     * Real commands have meaningful w1 values based on the command type. */
    if (w1 == 0x00007FD4 || w1 == 0x00007FD0) {
        /* Likely reading garbage memory - terminate this display list */
        GBI_DEBUG("[GBI] Detected garbage DL data at %p (w0=0x%08X w1=0x%08X), terminating\n",
                   (void*)cmd, w0, w1);
        return 0;  /* End display list */
    }

#if DEBUG_GBI
    /* Log interesting commands */
    if (opcode == G_FILLRECT || opcode == G_SETFILLCOLOR || opcode == G_SETOTHERMODE_H) {
        GBI_DEBUG("[GBI_CMD] cmd=%p FILL opcode=0x%02X w0=0x%08X w1=0x%08X\n",
               (void*)cmd, opcode, w0, w1);
    }
    /* Log branches/DL calls */
    if (opcode == 0x06 || opcode == G_ENDDL || opcode == 0xB8) {
        GBI_DEBUG("[GBI_CHAIN] cmd=%p op=0x%02X w1=0x%08X\n", (void*)cmd, opcode, w1);
    }
#endif

    switch (opcode) {
        case G_NOOP:
        case G_RDPPIPESYNC:
        case G_RDPTILESYNC:
        case G_RDPLOADSYNC:
        case G_RDPFULLSYNC:
        case G_SPNOOP:
            /* No-op commands */
            break;

        case G_ENDDL:  /* 0xDF - F3DEX2 format */
        case 0xB8:     /* Old F3D format: G_IMMFIRST-7 = -65-7 = -72 = 0xB8 */
            return 0; /* End display list */

        case G_CULLDL:  /* 0x03 - Cull Display List or G_MOVEMEM (F3D format) */
        {
            /* In F3D format, opcode 0x03 is shared between:
             * - G_MOVEMEM: w0 = 0x03 | (index << 16) | (length << 8) | offset
             * - G_CULLDL:  w0 = 0x03 | (vstart*2)
             *
             * We distinguish by checking if index byte (bits 16-23) matches
             * a known G_MV_* value. If so, it's G_MOVEMEM. */
            u8 index = (w0 >> 16) & 0xFF;

            GBI_DEBUG("[CMD 0x03] w0=0x%08X index=0x%02X (G_MV_VIEWPORT=0x%02X/0x80)\n",
                       w0, index, G_MV_VIEWPORT);

            /* Check if this is G_MOVEMEM (viewport, lights, etc.) */
            if (index == G_MV_VIEWPORT || index == 0x80) {
                void* data = seg2ptr(w1);
                gbi_cmd_set_viewport((Vp_t*)data);
                break;
            }
            /* TODO: Check for other G_MV_* indices (lights, etc.) */

            /* Otherwise, treat as G_CULLDL - just pass through */
            #if 0  /* Enable for debugging */
            uint32_t vstart = (w0 >> 1) & 0x7F;
            uint32_t vend = (w1 >> 1) & 0x7F;
            printf("[GBI] CULLDL: Verts %d to %d (culling disabled)\n", vstart, vend);
            #endif
            break;
        }

        /* Matrix commands - F3DEX2 format (0xDA) */
        case 0xDA:     /* F3DEX2: G_MTX */
            gbi_cmd_mtx(w0, w1);
            break;

        case G_POPMTX:
        case 0xBD:     /* Old F3D: G_IMMFIRST-2 = -67 = 0xBD */
            gbi_cmd_popmtx(w0, w1);
            break;

        /* Opcode 0x01 is AMBIGUOUS: G_VTX in F3DEX2, G_MTX in old F3D.
         * Distinguish by examining w0 encoding:
         * - G_VTX: w0 = (0x01 << 24) | (n << 12) | ((v0+n) << 1)
         *          n is in bits 19:12, vertex count 1-32
         * - G_MTX: w0 = (0x01 << 24) | params
         *          params is 0-7 (push/load/projection flags in bits 0-2)
         * Check: if bits 19:8 are non-zero, it's G_VTX; otherwise G_MTX */
        case 0x01:
            {
                int n_field = (w0 >> 12) & 0xFF;
                static int op01_debug = 0;
                if (op01_debug < 5) {
                    printf("[OPCODE 0x01] w0=0x%08X w1=0x%08X n_field=%d -> %s\n",
                           w0, w1, n_field, n_field > 0 ? "G_VTX" : "G_MTX");
                    fflush(stdout);
                    op01_debug++;
                }
                if (n_field > 0) {
                    /* Has vertex count - this is G_VTX */
                    gbi_cmd_vtx(w0, w1);
                } else {
                    /* No vertex count - this is old F3D G_MTX */
                    gbi_cmd_mtx(w0, w1);
                }
            }
            break;

        case 0x04:     /* Old F3D: G_VTX */
            gbi_cmd_vtx(w0, w1);
            break;

        /* Triangle commands */
        case 0x05:     /* F3DEX2: G_TRI1 */
        case 0xBF:     /* Old F3D: G_IMMFIRST-0 = -65 = 0xBF */
            gbi_cmd_tri1(w0, w1);
            break;

        case 0xBE:     /* Old F3D: G_IMMFIRST-14 = -79 = 0xBE (G_TRI2) */
            gbi_cmd_tri2(w0, w1);
            break;

        case 0xB1:     /* terrain_to_bin.py uses 0xB1 as G_TRI2 (standard 8-bit indices) */
            {
                extern int g_frame_count;
                static int b1_count = 0;
                if (g_frame_count == 185 && b1_count < 5) {
                    printf("[0xB1 HIT F185 #%d] w0=0x%08X w1=0x%08X\n", b1_count, w0, w1);
                    fflush(stdout);
                    b1_count++;
                }
            }
            gbi_cmd_tri2(w0, w1);
            break;

        case 0x0A:     /* G_TRIN_INDEPEND - Dolphin packed triangles */
            gbi_cmd_trin_independ(w0, w1);
            break;

        case G_TEXTURE:
            gbi_cmd_texture(w0, w1);
            break;

        case G_SETPRIMCOLOR:
            gbi_cmd_setprimcolor(w0, w1);
            break;

        case G_SETENVCOLOR:
            gbi_cmd_setenvcolor(w0, w1);
            break;

        case G_GEOMETRYMODE:  /* 0xD9 in F3DEX2 */
            gbi_cmd_geometrymode(w0, w1);
            break;

        case G_SETOTHERMODE_L:
            gbi_cmd_setothermode_l(w0, w1);
            break;

        case G_SETOTHERMODE_H:
        case 0xBA:  /* F3D format: G_SETOTHERMODE_H */
            gbi_cmd_setothermode_h(w0, w1);
            break;

        case G_SETTIMG:
            gbi_cmd_settimg(w0, w1);
            break;

        case G_DL:     /* 0xDE - F3DEX2 format */
        case 0x06:     /* Old F3D format: G_DL = 6 */
            /* Call sub-display list */
            if (w1 != 0) {
                /* Debug: track G_DL calls - look for cube address pattern */
                static int gdl_debug = 0;
                static u32 cube_addr = 0;
                /* Capture cube address on first gameplay frame (player cube init) */
                extern Gfx* player_cube_get_dl(void);
                Gfx* cube = player_cube_get_dl();
                if (cube) cube_addr = (u32)(uintptr_t)cube;
                /* Log if this G_DL matches the cube address */
                int is_cube_dl = (w1 == cube_addr && cube_addr != 0);
                if (is_cube_dl) {
                    static int cube_found = 0;
                    if (cube_found < 5) {
                        printf("[G_DL CUBE!!!] FOUND cube DL! w1=0x%08X cmd=%p\n",
                               w1, (void*)cmd);
                        cube_found++;
                    }
                } else if (gdl_debug < 5) {
                    /* Log first few G_DL for reference */
                    printf("[G_DL] w1=0x%08X (cube=0x%08X)\n", w1, cube_addr);
                    fflush(stdout);
                    gdl_debug++;
                }
                void* target = seg2ptr(w1);

                /* Frame 185 G_DL debug - trace BG_OPA traversal */
                if (g_frame_count == 185) {
                    static int f185_gdl_count = 0;
                    if (f185_gdl_count < 50) {
                        int nopush_flag;
                        if (opcode == 0x06) {
                            nopush_flag = (w0 >> 16) & 0xFF;
                        } else {
                            nopush_flag = w0 & 0x01;
                        }
                        printf("[F185 G_DL #%d] cmd=%p w1=0x%08X -> target=%p %s\n",
                               f185_gdl_count, (void*)cmd, w1, target,
                               nopush_flag ? "BRANCH" : "CALL");
                        fflush(stdout);
                        f185_gdl_count++;
                    }
                }

                /* Debug segment 10 (terrain) DL calls */
                if ((w1 >> 24) == 0x0A) {
                    static int seg10_dl_debug = 0;
                    if (seg10_dl_debug < 3) {
                        printf("[G_DL SEG10] w1=0x%08X -> target=%p seg[10]=0x%lX\n",
                               w1, target, (unsigned long)g_gbi_state.segments[10]);
                        if (target) {
                            Gfx* tgt_gfx = (Gfx*)target;
                            /* Print first 30 commands of terrain DL to find triangles */
                            for (int ci = 0; ci < 30; ci++) {
                                u8 op = tgt_gfx[ci].words.w0 >> 24;
                                printf("  terrain[%d]: (0x%08X, 0x%08X) op=0x%02X\n",
                                       ci, tgt_gfx[ci].words.w0, tgt_gfx[ci].words.w1, op);
                                /* Stop at EndDisplayList */
                                if (op == 0xDF || op == 0xB8) break;
                            }
                        }
                        fflush(stdout);
                        seg10_dl_debug++;
                    }
                }

                if (is_cube_dl) {
                    static int cube_exec = 0;
                    if (cube_exec < 5) {
                        Gfx* cube_gfx = (Gfx*)target;
                        printf("[CUBE_EXEC] seg2ptr returned %p (expected %p)\n",
                               target, (void*)cube);
                        if (cube_gfx) {
                            printf("[CUBE_EXEC] First cmd: (0x%08X, 0x%08X) op=0x%02X\n",
                                   cube_gfx->words.w0, cube_gfx->words.w1,
                                   cube_gfx->words.w0 >> 24);
                        }
                        fflush(stdout);
                        cube_exec++;
                    }
                    /* Set flag to track cube DL execution */
                    g_in_cube_dl = 1;
                }
#if DEBUG_GBI
                /* Debug segment addresses */
                if ((w1 >> 24) <= 0x0F && (w1 >> 24) != 0) {
                    int segnum = (w1 >> 24) & 0x0F;
                    GBI_DEBUG("[G_DL SEG] w1=0x%08X seg=%d -> target=%p base=0x%lX\n",
                               w1, segnum, target,
                               (unsigned long)g_gbi_state.segments[segnum]);
                }
#endif
                GBI_DEBUG("[G_DL] opcode=0x%02X w0=0x%08X w1=0x%08X -> target=%p\n",
                       opcode, w0, w1, target);
                /* Validate target pointer before executing */
                uintptr_t tgt_val = (uintptr_t)target;
                if (tgt_val < 0x10000 || tgt_val > 0x7fffffffffff) {
                    static int bad_gdl_warn = 0;
                    if (bad_gdl_warn < 10) {
                        printf("[G_DL BAD PTR] w1=0x%08X -> target=%p SKIPPING\n", w1, target);
                        fflush(stdout);
                        bad_gdl_warn++;
                    }
                    break;  /* Skip this G_DL command */
                }
                gbi_last_gdl_w1 = w1;  /* Record for history */
                gbi_execute((Gfx*)target);
                /* Reset cube flag after execution */
                if (is_cube_dl) {
                    g_in_cube_dl = 0;
                }
            }
            /* Check push flag - determines CALL vs BRANCH
             * G_DL_PUSH = 0x00 = Call (push return address, continue after return)
             * G_DL_NOPUSH = 0x01 = Branch (don't push, end current list after branch)
             */
            {
                int nopush;
                if (opcode == 0x06) {
                    /* F3D format: push flag in bits 16-23 of w0 */
                    nopush = (w0 >> 16) & 0xFF;
                } else {
                    /* F3DEX2 format: push flag in bit 0 of w0 */
                    nopush = w0 & 0x01;
                }
                if (nopush) {
                    /* Debug: log when we branch out */
                    static int branch_debug = 0;
                    if (branch_debug < 30) {
                        printf("[G_DL BRANCH] Ending this DL, branched to w1=0x%08X\n", w1);
                        fflush(stdout);
                        branch_debug++;
                    }
                    return 0; /* Branch - end this list */
                }
            }
            break;

        case G_MOVEWORD:
            /* F3DEX2 MoveWord (0xDB) */
            gbi_cmd_moveword(w0, w1);
            break;

        case 0xBC:
            /* Old GBI MoveWord (G_IMMFIRST - 3 = 0xBC) */
            gbi_cmd_moveword(w0, w1);
            break;

        case G_SETSCISSOR:
            /* Set scissor rectangle (0xED) */
            gbi_cmd_setscissor(w0, w1);
            break;

        case G_MOVEMEM:  /* G_MOVEMEM (F3DEX2 = 0xDC) */
        {
            /* Move memory - viewport, lights, etc. */
            u8 index = (w0 >> 16) & 0xFF;
            void* data = seg2ptr(w1);

            switch (index) {
                case G_MV_VIEWPORT:  /* = 8 (F3DEX2) or 0x80 (F3D) */
                case 0x80:           /* F3D version */
                    gbi_cmd_set_viewport((Vp_t*)data);
                    break;
                /* TODO: Handle G_MV_LIGHT, G_MV_MATRIX, etc. */
                default:
                    break;
            }
            break;
        }

        case G_SETCOMBINE:
            /* Set color combiner - complex, stub for now */
            break;

        case G_SETFOGCOLOR:
            g_gbi_state.fog_color[0] = (w1 >> 24) & 0xFF;
            g_gbi_state.fog_color[1] = (w1 >> 16) & 0xFF;
            g_gbi_state.fog_color[2] = (w1 >> 8) & 0xFF;
            g_gbi_state.fog_color[3] = w1 & 0xFF;
            break;

        case G_SETBLENDCOLOR:
            g_gbi_state.blend_color[0] = (w1 >> 24) & 0xFF;
            g_gbi_state.blend_color[1] = (w1 >> 16) & 0xFF;
            g_gbi_state.blend_color[2] = (w1 >> 8) & 0xFF;
            g_gbi_state.blend_color[3] = w1 & 0xFF;
            break;

        case G_SETFILLCOLOR:
            /* Fill color is packed as two 16-bit RGBA5551 values for fill mode.
             * We only need one of them (they're identical), use high 16 bits.
             * RGBA5551 format: RRRRR GGGGG BBBBB A */
            {
                u16 rgba5551 = (w1 >> 16) & 0xFFFF;
                /* Extract and expand each component to 8-bit */
                g_gbi_state.fill_color[0] = ((rgba5551 >> 11) & 0x1F) * 255 / 31;  /* R */
                g_gbi_state.fill_color[1] = ((rgba5551 >> 6) & 0x1F) * 255 / 31;   /* G */
                g_gbi_state.fill_color[2] = ((rgba5551 >> 1) & 0x1F) * 255 / 31;   /* B */
                g_gbi_state.fill_color[3] = (rgba5551 & 0x01) ? 255 : 0;           /* A */
                static int fill_debug = 0;
                if (fill_debug++ < 5) {
                    printf("[FILL_COLOR] w1=0x%08X rgba5551=0x%04X -> R=%d G=%d B=%d A=%d\n",
                           w1, rgba5551,
                           g_gbi_state.fill_color[0], g_gbi_state.fill_color[1],
                           g_gbi_state.fill_color[2], g_gbi_state.fill_color[3]);
                    fflush(stdout);
                }
            }
            break;

        case G_FILLRECT:
            /* Fill rectangle - N64 format (PR/gbi.h):
             * Format: w0 = opcode | (lrx << 14) | (lry << 2)
             *         w1 = (ulx << 14) | (uly << 2)
             * Coordinates are 10-bit integer pixels at positions 23:14 and 11:2 */
            {
                int lrx = (w0 >> 14) & 0x3FF;  /* bits 23:14, 10 bits */
                int lry = (w0 >> 2) & 0x3FF;   /* bits 11:2, 10 bits */
                int ulx = (w1 >> 14) & 0x3FF;  /* bits 23:14, 10 bits */
                int uly = (w1 >> 2) & 0x3FF;   /* bits 11:2, 10 bits */

                static int fill_rect_debug = 0;
                if (fill_rect_debug++ < 5) {
                    printf("[FILLRECT] ulx=%d uly=%d lrx=%d lry=%d color=(%d,%d,%d,%d)\n",
                           ulx, uly, lrx, lry,
                           g_gbi_state.fill_color[0], g_gbi_state.fill_color[1],
                           g_gbi_state.fill_color[2], g_gbi_state.fill_color[3]);
                    fflush(stdout);
                }

                /* Draw filled rectangle with current fill color */
                glDisable(GL_TEXTURE_2D);
                glColor4ub(g_gbi_state.fill_color[0], g_gbi_state.fill_color[1],
                          g_gbi_state.fill_color[2], g_gbi_state.fill_color[3]);
                glBegin(GL_QUADS);
                glVertex2i(ulx, uly);
                glVertex2i(lrx, uly);
                glVertex2i(lrx, lry);
                glVertex2i(ulx, lry);
                glEnd();
            }
            break;

        case G_SETTILE:
            gbi_cmd_settile(w0, w1);
            break;

        case 0xD2:     /* G_SETTILE_DOLPHIN - Dolphin/GC tile setup */
            gbi_cmd_settile_dolphin(w0, w1);
            break;

        case G_LOADTILE:
            gbi_cmd_loadtile(w0, w1);
            break;

        case G_LOADBLOCK:
            gbi_cmd_loadblock(w0, w1);
            break;

        case G_SETTILESIZE:
            gbi_cmd_settilesize(w0, w1);
            break;

        case G_LOADTLUT:
            gbi_cmd_loadtlut(w0, w1);
            break;

        case 0xDD:     /* G_LOADTLUT_DOLPHIN - Dolphin/GC palette load */
            gbi_cmd_loadtlut_dolphin(w0, w1);
            break;

        case 0xD0:     /* G_LOADTEXBLOCK_DOLPHIN - Dolphin/GC texture load */
            gbi_cmd_loadtexblock_dolphin(w0, w1);
            break;

        case G_SETCIMG:
        case G_SETZIMG:
            /* Set color/Z image - framebuffer setup */
            break;

        case G_RDPSETOTHERMODE:
            /* Set both othermode words at once */
            g_gbi_state.othermode_high = w0 & 0x00FFFFFF;
            g_gbi_state.othermode_low = w1;
            break;

        /* Note: F3DEX2 values conflict with old F3D:
         * G_CULLDL=0x03, G_BRANCH_Z=0x04, G_MODIFYVTX=0x02
         * Old F3D uses different encodings - ignore these F3DEX2 values
         * since they conflict with MTX(0x01), VTX(0x04), MOVEMEM(0x03) */
        case 0x02:     /* F3DEX2 G_MODIFYVTX - unused in old F3D */
            break;

        case 0xBB:     /* Unknown command - appears in display lists, stub as no-op */
            /* Possibly G_IMMFIRST-4 in old F3D, or custom command */
            /* w1=0xFFFFFFFF often seen - might be clearing some state */
            break;

        /* libforest extension commands - stub for now */
        case 0xCE:     /* G_SETTEXEDGEALPHA - edge alpha for anti-aliasing */
            /* w1 contains alpha threshold value */
            break;

        case 0xD5:     /* G_SPECIAL_1 - libforest special command */
            /* Used for texture/material related operations */
            break;

        default:
            /* Unhandled command - log for debugging */
            printf("GBI: Unhandled opcode 0x%02X (w0=0x%08X, w1=0x%08X)\n",
                   opcode, w0, w1);
            break;
    }

    return 1; /* Continue processing */
}

/**
 * Execute a display list
 */
static int total_cmds_processed = 0;
static int recursion_depth = 0;

/* Recursion guard: Track last 16 display list calls for debugging infinite loops */
#define GBI_HISTORY_SIZE 16
static struct {
    Gfx* dl_addr;           /* Display list address */
    u32 seg_addr;           /* Original segmented address (w1 from G_DL) */
    u8 first_opcode;        /* First command's opcode */
    u8 depth;               /* Recursion depth when entered */
} gbi_dl_history[GBI_HISTORY_SIZE];
static int gbi_history_idx = 0;
static int gbi_recursion_guard_triggered = 0;

static void gbi_print_dl_history(void) {
    printf("\n[GBI RECURSION GUARD] Display list call history (last %d):\n",
           GBI_HISTORY_SIZE);
    printf("  %-4s %-18s %-10s %-10s %s\n", "Idx", "DL Address", "Seg Addr", "Opcode", "Depth");
    printf("  " "----" " " "------------------" " " "----------" " " "----------" " " "-----\n");
    for (int i = 0; i < GBI_HISTORY_SIZE; i++) {
        int idx = (gbi_history_idx + i) % GBI_HISTORY_SIZE;
        if (gbi_dl_history[idx].dl_addr) {
            printf("  [%2d] %p  0x%08X   0x%02X       %d\n",
                   i, (void*)gbi_dl_history[idx].dl_addr,
                   gbi_dl_history[idx].seg_addr,
                   gbi_dl_history[idx].first_opcode,
                   gbi_dl_history[idx].depth);
        }
    }
    printf("\n");
    fflush(stdout);
}

/* Check if this DL is already in the call stack (circular reference) */
static int gbi_detect_circular_dl(Gfx* dl) {
    /* Only check entries at depths <= current depth (active call stack) */
    for (int i = 0; i < GBI_HISTORY_SIZE; i++) {
        if (gbi_dl_history[i].dl_addr == dl &&
            gbi_dl_history[i].depth < recursion_depth) {
            return 1;  /* Circular reference detected */
        }
    }
    return 0;
}

void gbi_execute(Gfx* dl) {
    if (!dl) return;

    /* Frame debug for GBI execution */
    extern int g_frame_count;
    static int gbi_exec_debug = 0;
    if (g_frame_count >= 180 && g_frame_count <= 182 && gbi_exec_debug < 3) {
        printf("[GBI_EXEC F%d] dl=%p\n", g_frame_count, (void*)dl);
        fflush(stdout);
        gbi_exec_debug++;
    }

    /* Validate pointer is in reasonable memory range to catch garbage pointers.
     * Valid pointers should be in user space (below 0x7fff00000000 on Linux).
     * Very low pointers (< 0x10000) are also likely garbage. */
    uintptr_t ptr_val = (uintptr_t)dl;
    if (ptr_val < 0x10000 || ptr_val > 0x7fffffffffff) {
        static int bad_ptr_warn = 0;
        if (bad_ptr_warn < 3) {
            printf("[GBI] WARNING: Invalid display list pointer %p, skipping\n", (void*)dl);
            fflush(stdout);
            bad_ptr_warn++;
        }
        return;
    }

    /* Check for circular reference BEFORE incrementing depth */
    if (gbi_detect_circular_dl(dl)) {
        if (!gbi_recursion_guard_triggered) {
            gbi_recursion_guard_triggered = 1;
            printf("\n[GBI RECURSION GUARD] CIRCULAR REFERENCE DETECTED!\n");
            printf("  DL %p is already in the call stack at depth %d\n",
                   (void*)dl, recursion_depth);
            gbi_print_dl_history();
        }
        return;  /* Don't follow the circular reference */
    }

    recursion_depth++;

    /* Record this DL in history */
    gbi_dl_history[gbi_history_idx].dl_addr = dl;
    gbi_dl_history[gbi_history_idx].seg_addr = gbi_last_gdl_w1;
    gbi_dl_history[gbi_history_idx].first_opcode = (dl->words.w0 >> 24) & 0xFF;
    gbi_dl_history[gbi_history_idx].depth = recursion_depth;
    gbi_history_idx = (gbi_history_idx + 1) % GBI_HISTORY_SIZE;

    if (recursion_depth > 16) {
        if (!gbi_recursion_guard_triggered) {
            gbi_recursion_guard_triggered = 1;
            printf("\n[GBI RECURSION GUARD] MAX DEPTH (16) EXCEEDED!\n");
            printf("  Current DL: %p, opcode: 0x%02X\n",
                   (void*)dl, (dl->words.w0 >> 24) & 0xFF);
            gbi_print_dl_history();
        }
        recursion_depth--;
        return;
    }

    /* Log all display list entries for gameplay frames */
    static int gbi_frame_log = 0;
    if (recursion_depth == 1) gbi_frame_log++;
    /* Log frames 180-190 (gameplay phase where player cube is drawn) */
    if (gbi_frame_log >= 180 && gbi_frame_log <= 195 && recursion_depth == 1) {
        printf("[GBI_EXEC] frame=%d dl=%p first_cmd=(0x%08X,0x%08X)\n",
               gbi_frame_log, (void*)dl,
               dl ? dl->words.w0 : 0, dl ? dl->words.w1 : 0);
        fflush(stdout);
    }

    /* Increment debug frame at top-level only */
    if (recursion_depth == 1) {
        g_gbi_debug_frame++;

        /* Reset recursion guard for new frame */
        gbi_recursion_guard_triggered = 0;
        gbi_history_idx = 0;
        for (int i = 0; i < GBI_HISTORY_SIZE; i++) {
            gbi_dl_history[i].dl_addr = NULL;
        }

        /* Reset matrix state at frame start.
         * N64 expects matrices to be set explicitly by the display list
         * each frame, so we start with identity matrices. */
        mtx_identity(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr]);
        mtx_identity(g_gbi_state.modelview_stack[0]);
        g_gbi_state.modelview_stack_ptr = 0;
        g_gbi_state.combined_mtx_dirty = 1;

        /* Reset OpenGL matrices at frame start.
         * Set up orthographic projection to map N64 screen coords to OpenGL.
         * N64 outputs screen coords in 11.2 fixed-point format (scaled by 4).
         * After dividing by 4, we get pixel coordinates (0-640, 0-480).
         *
         * For top-down terrain view, we use Z as Y (world Z -> screen Y).
         * The terrain Z range in pixels is roughly 150-490.
         * Set up orthographic projection to map these ranges to NDC. */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        /* Map pixel X (0-640) to NDC X (-1 to 1)
         * Map pixel Z (0-640) to NDC Y (-1 to 1) with Y-flip
         * Use larger near/far to accommodate 3D objects like player cube */
        glOrtho(0, 640, 640, 0, -100, 100);  /* left, right, bottom, top, near, far */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Apply any pending translation (for positioned objects like player) */
        gbi_apply_translation();
    }

    g_gbi_state.display_lists_executed++;

    /* Track terrain DL execution at frame 185 */
    extern int g_frame_count;
    int is_terrain_dl = 0;
    static int terrain_dl_exec_count = 0;
    if (g_frame_count == 185 && terrain_dl_exec_count < 5) {
        /* Check if this looks like a terrain DL (starts with G_TEXTURE 0xD7) */
        if ((dl->words.w0 >> 24) == 0xD7) {
            is_terrain_dl = 1;
            printf("[TERRAIN_DL_EXEC #%d] Entry dl=%p first_cmd=(0x%08X, 0x%08X)\n",
                   terrain_dl_exec_count, (void*)dl, dl->words.w0, dl->words.w1);
            fflush(stdout);
        }
    }

    /* Process commands until G_ENDDL */
    int cmd_count = 0;
    while (gbi_execute_cmd(dl)) {
        dl++;
        cmd_count++;
        if (cmd_count > 10000) {
            /* Safety limit */
            break;
        }
    }
    total_cmds_processed += cmd_count;

    /* Log terrain DL command count */
    if (is_terrain_dl) {
        printf("[TERRAIN_DL_EXEC #%d] Exit: processed %d commands\n",
               terrain_dl_exec_count, cmd_count);
        fflush(stdout);
        terrain_dl_exec_count++;
    }

    /* DIAGNOSTIC: Draw a test triangle to verify OpenGL is working.
     * This uses fixed NDC coords (-1 to 1) with identity matrices.
     * DISABLED - terrain should now be visible */
    if (0 && recursion_depth == 1 && g_gbi_debug_frame >= 30 && g_gbi_debug_frame <= 60) {
        /* Save current matrices */
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        /* Draw a bright red triangle in the corner */
        glDisable(GL_DEPTH_TEST);
        printf("[TEST TRIANGLE] Drawing at frame %d\n", g_gbi_debug_frame);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);  /* Bright red */
        glVertex3f(-0.9f, -0.9f, 0.0f);
        glVertex3f(-0.7f, -0.9f, 0.0f);
        glVertex3f(-0.8f, -0.7f, 0.0f);
        glEnd();
        glEnable(GL_DEPTH_TEST);

        /* Restore matrices */
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
    if (g_gbi_state.display_lists_executed <= 5 || g_gbi_state.display_lists_executed % 60 == 0) {
        printf("[GBI] Display list %d: %d commands (total: %d)\n",
               g_gbi_state.display_lists_executed, cmd_count, total_cmds_processed);
    }

    /* Print opcode summary at frame 60 and 120 */
    if ((g_gbi_debug_frame == 60 || g_gbi_debug_frame == 120) && !opcode_summary_printed) {
        printf("\n[OPCODE SUMMARY] frame=%d:\n", g_gbi_debug_frame);
        for (int i = 0; i < 256; i++) {
            if (opcode_count[i] > 0) {
                const char* name = "?";
                switch(i) {
                    case 0x00: name = "G_NOOP"; break;
                    case 0x01: name = "G_MTX(F3D)"; break;
                    case 0x04: name = "G_VTX"; break;
                    case 0x05: name = "G_TRI1(F3DEX2)"; break;
                    case 0x06: name = "G_DL(F3D)"; break;
                    case 0xB6: name = "G_CLEARGEOMETRYMODE"; break;
                    case 0xB7: name = "G_SETGEOMETRYMODE"; break;
                    case 0xB8: name = "G_ENDDL(F3D)"; break;
                    case 0xBA: name = "G_SETOTHERMODE_H"; break;
                    case 0xBB: name = "G_SETOTHERMODE_L"; break;
                    case 0xBC: name = "G_MOVEWORD"; break;
                    case 0xBD: name = "G_POPMTX"; break;
                    case 0xBE: name = "G_TRI2"; break;
                    case 0xBF: name = "G_TRI1(F3D)"; break;
                    case 0xDA: name = "G_MTX(F3DEX2)"; break;
                    case 0xDE: name = "G_DL(F3DEX2)"; break;
                    case 0xDF: name = "G_ENDDL(F3DEX2)"; break;
                    case 0xE4: name = "G_TEXRECT"; break;
                    case 0xE6: name = "G_RDPLOADSYNC"; break;
                    case 0xE7: name = "G_RDPPIPESYNC"; break;
                    case 0xE8: name = "G_RDPTILESYNC"; break;
                    case 0xE9: name = "G_RDPFULLSYNC"; break;
                    case 0xED: name = "G_SETSCISSOR"; break;
                    case 0xF0: name = "G_LOADTLUT"; break;
                    case 0xDD: name = "G_LOADTLUT_DOLPHIN"; break;
                    case 0xD0: name = "G_LOADTEXBLOCK_DOLPHIN"; break;
                    case 0xD2: name = "G_SETTILE_DOLPHIN"; break;
                    case 0xF2: name = "G_SETTILESIZE"; break;
                    case 0xF3: name = "G_LOADBLOCK"; break;
                    case 0xF4: name = "G_LOADTILE"; break;
                    case 0xF5: name = "G_SETTILE"; break;
                    case 0xF6: name = "G_FILLRECT"; break;
                    case 0xF7: name = "G_SETFILLCOLOR"; break;
                    case 0xF8: name = "G_SETFOGCOLOR"; break;
                    case 0xF9: name = "G_SETBLENDCOLOR"; break;
                    case 0xFA: name = "G_SETPRIMCOLOR"; break;
                    case 0xFB: name = "G_SETENVCOLOR"; break;
                    case 0xFC: name = "G_SETCOMBINE"; break;
                    case 0xFD: name = "G_SETTIMG"; break;
                    case 0xFE: name = "G_SETZIMG"; break;
                    case 0xFF: name = "G_SETCIMG"; break;
                }
                printf("  0x%02X %-20s: %d\n", i, name, opcode_count[i]);
            }
        }
        printf("\n");
        fflush(stdout);
        if (g_gbi_debug_frame == 120) opcode_summary_printed = 1;
    }
    recursion_depth--;
}

/**
 * Test function: Draw a triangle using the GBI vertex pipeline
 * This verifies that G_VTX and G_TRI1 commands work correctly.
 * Called at frame 150+ when past the trademark screen.
 */
static int test_triangle_drawn = 0;

void gbi_draw_test_triangle(void) {
    if (test_triangle_drawn || g_gbi_debug_frame < 150) return;

    /* Test vertex data - a simple triangle in screen coordinates */
    static Vtx test_verts[3] = {
        /* Position (x, y, z, flag), TC (s, t), Color/Normal (r, g, b, a) */
        { .v = { .ob = { 320, 100, 0 }, .flag = 0, .tc = { 0, 0 }, .cn = { 0, 255, 0, 255 } } },  /* Top - Green */
        { .v = { .ob = { 220, 300, 0 }, .flag = 0, .tc = { 0, 0 }, .cn = { 255, 0, 0, 255 } } },  /* Bottom left - Red */
        { .v = { .ob = { 420, 300, 0 }, .flag = 0, .tc = { 0, 0 }, .cn = { 0, 0, 255, 255 } } },  /* Bottom right - Blue */
    };

    /* Build a simple display list */
    static Gfx test_dl[10];
    Gfx* gfx = test_dl;

    /* Set segment 0 to NULL (direct addresses) */
    gfx->words.w0 = (0xBC << 24) | (0x06 << 16) | 0x0000;  /* G_MOVEWORD, G_MW_SEGMENT, seg 0 */
    gfx->words.w1 = 0;
    gfx++;

    /* Load 3 vertices at buffer index 0 */
    /* G_VTX format: w0 = (0x04 << 24) | (n << 12) | (vn << 1) */
    /*               w1 = address */
    gfx->words.w0 = (0x04 << 24) | (3 << 12) | ((0 + 3) << 1);  /* n=3, v0=0 */
    gfx->words.w1 = (u32)(uintptr_t)test_verts;
    gfx++;

    /* Draw triangle with vertices 0, 1, 2 */
    /* G_TRI1 format (F3D): w0 = (0xBF << 24), w1 = (v0*2 << 16) | (v1*2 << 8) | (v2*2) */
    gfx->words.w0 = (0xBF << 24);
    gfx->words.w1 = (0 << 16) | (2 << 8) | 4;  /* v0=0, v1=1, v2=2 (indices * 2) */
    gfx++;

    /* End display list */
    gfx->words.w0 = (0xB8 << 24);  /* G_ENDDL (F3D) */
    gfx->words.w1 = 0;

    /* Set up orthographic projection for 2D test */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* Set identity matrices in GBI state */
    mtx_identity(g_gbi_state.projection_stack[g_gbi_state.projection_stack_ptr]);
    mtx_identity(g_gbi_state.modelview_stack[0]);
    g_gbi_state.modelview_stack_ptr = 0;
    g_gbi_state.combined_mtx_dirty = 1;

    /* Disable lighting to use vertex colors */
    g_gbi_state.geometry_mode = 0;

    /* Disable depth test for 2D */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    printf("\n[TEST] Drawing test triangle via GBI pipeline at frame %d\n", g_gbi_debug_frame);
    printf("[TEST] Vertices at %p: (320,100), (220,300), (420,300)\n", (void*)test_verts);
    fflush(stdout);

    /* Execute the test display list */
    gbi_execute(test_dl);

    printf("[TEST] Test triangle display list executed\n");
    fflush(stdout);

    test_triangle_drawn = 1;
}

/**
 * Set a translation offset for the next gbi_execute call.
 * This positions actors in world space before rendering their display list.
 */
void gbi_set_translation(f32 x, f32 y, f32 z) {
    g_gbi_translation.x = x;
    g_gbi_translation.y = y;
    g_gbi_translation.z = z;
    g_gbi_translation.active = 1;

    /* Debug output for first few calls */
    static int trans_debug = 0;
    if (trans_debug < 5) {
        printf("[GBI] Set translation: (%.1f, %.1f, %.1f)\n", x, y, z);
        fflush(stdout);
        trans_debug++;
    }
}

/**
 * Clear the translation offset (called after display list execution).
 */
void gbi_clear_translation(void) {
    g_gbi_translation.x = 0;
    g_gbi_translation.y = 0;
    g_gbi_translation.z = 0;
    g_gbi_translation.active = 0;
}

/**
 * Apply the current translation to OpenGL modelview matrix.
 * Call this before executing a display list that needs positioning.
 */
void gbi_apply_translation(void) {
    if (g_gbi_translation.active) {
        /* Scale world coordinates to screen coordinates.
         * Terrain uses ~5120 world units = 320 screen pixels.
         * So scale factor is roughly 320/5120 = 0.0625 */
        f32 scale = 320.0f / 5120.0f;
        f32 screen_x = g_gbi_translation.x * scale;
        f32 screen_z = g_gbi_translation.z * scale;  /* Z maps to Y on screen */

        static int trans_apply_debug = 0;
        if (trans_apply_debug < 5) {
            printf("[GBI] Applying translation: world(%.1f,%.1f,%.1f) -> screen(%.1f,%.1f)\n",
                   g_gbi_translation.x, g_gbi_translation.y, g_gbi_translation.z,
                   screen_x, screen_z);
            fflush(stdout);
            trans_apply_debug++;
        }

        glMatrixMode(GL_MODELVIEW);
        glTranslatef(screen_x, screen_z, 0.0f);
    }
}
