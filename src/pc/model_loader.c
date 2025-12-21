/**
 * @file model_loader.c
 * @brief Binary Model Loader for PC Port
 *
 * Loads pre-extracted binary vertex and display list data at runtime,
 * patching 32-bit pointers to valid 64-bit addresses.
 *
 * This solves the "initializer element is not constant" problem that
 * prevents compiling original model .c files on 64-bit systems.
 *
 * Usage:
 *   1. Extract model data using tools/extract_model.py
 *   2. Place .vtx files in assets/models/
 *   3. Call model_loader_init() at startup
 *   4. Call model_loader_get_*() to retrieve patched pointers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "pc/platform.h"
#include "pc/gbi.h"

/* External pointer registry functions */
extern void ptr_registry_add(void* ptr64, const char* name);
extern void ptr_registry_add_range(void* base, size_t size, const char* name);

/* ============================================================================
 * Binary Format Definitions
 * ============================================================================ */

/* Vertex format (16 bytes, matches N64 Vtx structure) */
typedef struct {
    int16_t x, y, z;    /* Position */
    uint16_t flag;      /* Flag */
    int16_t s, t;       /* Texture coords */
    uint8_t r, g, b, a; /* Color/normal + alpha */
} ModelVertex;

/* GBI command (8 bytes) */
typedef struct {
    uint32_t w0;
    uint32_t w1;
} ModelGfxCmd;

/* Loaded model data */
typedef struct {
    char name[64];
    Vtx* vertices;
    int vertex_count;
    Gfx* display_list;
    int dl_command_count;
    int loaded;
} LoadedModel;

/* ============================================================================
 * Endian Conversion
 * ============================================================================ */

static inline uint16_t swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

static inline int16_t swap16s(int16_t val) {
    uint16_t u = (uint16_t)val;
    return (int16_t)swap16(u);
}

static inline uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

/* ============================================================================
 * Model Registry
 * ============================================================================ */

#define MAX_LOADED_MODELS 64
static LoadedModel g_models[MAX_LOADED_MODELS];
static int g_model_count = 0;

/* ============================================================================
 * Binary Loading Functions
 * ============================================================================ */

/**
 * Load binary vertex data from file
 */
static Vtx* load_vertices_binary(const char* filepath, int* out_count) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("[MODEL_LOADER] Could not open: %s\n", filepath);
        return NULL;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    int count = size / sizeof(ModelVertex);
    if (count == 0) {
        fclose(f);
        return NULL;
    }

    /* Allocate vertex buffer */
    Vtx* vertices = (Vtx*)malloc(count * sizeof(Vtx));
    if (!vertices) {
        fclose(f);
        return NULL;
    }

    /* Read and convert each vertex */
    ModelVertex raw;
    for (int i = 0; i < count; i++) {
        if (fread(&raw, sizeof(ModelVertex), 1, f) != 1) {
            printf("[MODEL_LOADER] Read error at vertex %d\n", i);
            free(vertices);
            fclose(f);
            return NULL;
        }

        /* Convert from big-endian to native (little-endian on x86) */
        vertices[i].v.ob[0] = swap16s(raw.x);
        vertices[i].v.ob[1] = swap16s(raw.y);
        vertices[i].v.ob[2] = swap16s(raw.z);
        vertices[i].v.flag = swap16(raw.flag);
        vertices[i].v.tc[0] = swap16s(raw.s);
        vertices[i].v.tc[1] = swap16s(raw.t);
        vertices[i].v.cn[0] = raw.r;
        vertices[i].v.cn[1] = raw.g;
        vertices[i].v.cn[2] = raw.b;
        vertices[i].v.cn[3] = raw.a;
    }

    fclose(f);
    *out_count = count;

    printf("[MODEL_LOADER] Loaded %d vertices from %s\n", count, filepath);
    return vertices;
}

/* ============================================================================
 * Runtime Display List Building
 * ============================================================================ */

/**
 * Build a display list at runtime for the dump model.
 * This recreates the structure of dump_s_DL_model using runtime gsSP* calls.
 */
static Gfx* build_dump_displaylist(Vtx* vertices, int vertex_count, u8* tex1, u8* tex2) {
    /* Allocate space for the display list (generous estimate) */
    Gfx* dl = (Gfx*)malloc(256 * sizeof(Gfx));
    if (!dl) return NULL;

    Gfx* p = dl;

    /* Based on obj_s_dump.c structure:
     * dump_s_DL_model calls obj_s_dump_t1T_model and obj_s_dump_t2T_model
     *
     * For now, we create a simplified display list that just draws the vertices
     * with basic settings. Full texture support requires texture loading too.
     */

    /* Setup texture and render mode */
    *p++ = (Gfx){{ 0xD7000000, 0x00010001 }};  /* gsSPTexture(0,0,0,0,G_ON) */

    /* Set geometry mode */
    *p++ = (Gfx){{ 0xD9000000, 0x00220405 }};  /* gsSPLoadGeometryMode(...) */

    /* Set prim color (white with full alpha) */
    *p++ = (Gfx){{ 0xFA000080, 0xFFFFFFFF }};  /* gsDPSetPrimColor(0,128,255,255,255,255) */

    /* Load first batch of vertices (32 max per call) */
    if (vertex_count > 0) {
        int batch1 = (vertex_count > 32) ? 32 : vertex_count;
        p->w0 = 0x01000000 | ((batch1 & 0xFF) << 12) | ((batch1 * 2) << 1);
        p->w1 = (u32)(uintptr_t)vertices;
        p++;

        /* Draw triangles for first 32 vertices (quads as triangle pairs) */
        /* Simplified: draw as triangle strip/fan pattern */
        for (int i = 0; i < batch1 - 2 && i < 30; i += 4) {
            /* gsSP2Triangles equivalent */
            p->w0 = 0x06000000;  /* G_TRI2 */
            p->w1 = ((i << 17) | ((i+1) << 9) | ((i+2) << 1)) << 8 |
                    (((i+2) << 17) | ((i+3) << 9) | ((i) << 1));
            p++;
        }
    }

    /* Load second batch if needed */
    if (vertex_count > 32) {
        int batch2_start = 32;
        int batch2_count = vertex_count - 32;
        if (batch2_count > 32) batch2_count = 32;

        p->w0 = 0x01000000 | ((batch2_count & 0xFF) << 12) | ((batch2_count * 2) << 1);
        p->w1 = (u32)(uintptr_t)&vertices[batch2_start];
        p++;
    }

    /* End display list */
    *p++ = (Gfx){{ 0xDF000000, 0x00000000 }};  /* gsSPEndDisplayList */

    int cmd_count = (int)(p - dl);
    printf("[MODEL_LOADER] Built display list with %d commands\n", cmd_count);

    return dl;
}

/* ============================================================================
 * Dump Model Loader (Specific Implementation)
 * ============================================================================ */

/* Storage for dump model data */
static Vtx* g_dump_vertices = NULL;
static int g_dump_vertex_count = 0;
static Gfx* g_dump_s_dl = NULL;
static Gfx* g_dump_w_dl = NULL;
static int g_dump_loaded = 0;

/* Textures (placeholder for now) */
static u8 g_dump_tex1[2048] = {0};  /* Placeholder texture */
static u8 g_dump_tex2[2048] = {0};

/**
 * Load the dump model from binary vertex data
 */
int model_loader_load_dump(void) {
    if (g_dump_loaded) {
        return 1;  /* Already loaded */
    }

    const char* vtx_paths[] = {
        "assets/models/dump_s_obj_s_dump_v.vtx",
        "../assets/models/dump_s_obj_s_dump_v.vtx",
        "../../assets/models/dump_s_obj_s_dump_v.vtx",
    };

    /* Try to load vertices */
    for (int i = 0; i < 3; i++) {
        g_dump_vertices = load_vertices_binary(vtx_paths[i], &g_dump_vertex_count);
        if (g_dump_vertices) {
            printf("[MODEL_LOADER] Dump vertices loaded from %s\n", vtx_paths[i]);
            break;
        }
    }

    if (!g_dump_vertices) {
        printf("[MODEL_LOADER] Could not load dump vertex data\n");
        return 0;
    }

    /* Register vertex data with pointer registry */
    ptr_registry_add_range(g_dump_vertices, g_dump_vertex_count * sizeof(Vtx), "dump_vertices");

    /* Build display lists at runtime */
    g_dump_s_dl = build_dump_displaylist(g_dump_vertices, g_dump_vertex_count, g_dump_tex1, g_dump_tex2);
    if (!g_dump_s_dl) {
        printf("[MODEL_LOADER] Failed to build dump display list\n");
        return 0;
    }

    /* Winter uses same geometry */
    g_dump_w_dl = g_dump_s_dl;

    /* Register display lists */
    ptr_registry_add((void*)g_dump_s_dl, "dump_s_DL");

    g_dump_loaded = 1;
    printf("[MODEL_LOADER] Dump model loaded successfully!\n");
    return 1;
}

/**
 * Get dump summer display list
 */
static int g_dump_dl_get_count = 0;
Gfx* model_loader_get_dump_s_dl(void) {
    if (!g_dump_loaded) {
        model_loader_load_dump();
    }
    if (g_dump_dl_get_count < 5 && g_dump_s_dl != NULL) {
        printf("[MODEL_LOADER] get_dump_s_dl() returning DL at %p\n", (void*)g_dump_s_dl);
        g_dump_dl_get_count++;
    }
    return g_dump_s_dl;
}

/**
 * Get dump winter display list
 */
Gfx* model_loader_get_dump_w_dl(void) {
    if (!g_dump_loaded) {
        model_loader_load_dump();
    }
    return g_dump_w_dl;
}

/**
 * Get dump vertices
 */
Vtx* model_loader_get_dump_vertices(void) {
    if (!g_dump_loaded) {
        model_loader_load_dump();
    }
    return g_dump_vertices;
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * Initialize the model loader system
 */
void model_loader_init(void) {
    printf("[MODEL_LOADER] Initializing model loader...\n");
    memset(g_models, 0, sizeof(g_models));
    g_model_count = 0;

    /* Pre-load critical models */
    model_loader_load_dump();

    printf("[MODEL_LOADER] Model loader initialized\n");
}

/**
 * Shutdown and free all loaded models
 */
void model_loader_shutdown(void) {
    printf("[MODEL_LOADER] Shutting down...\n");

    if (g_dump_vertices) {
        free(g_dump_vertices);
        g_dump_vertices = NULL;
    }
    if (g_dump_s_dl && g_dump_s_dl != g_dump_w_dl) {
        free(g_dump_s_dl);
    }
    if (g_dump_w_dl) {
        free(g_dump_w_dl);
    }
    g_dump_s_dl = NULL;
    g_dump_w_dl = NULL;
    g_dump_loaded = 0;

    for (int i = 0; i < g_model_count; i++) {
        if (g_models[i].vertices) {
            free(g_models[i].vertices);
        }
        if (g_models[i].display_list) {
            free(g_models[i].display_list);
        }
    }
    g_model_count = 0;
}
