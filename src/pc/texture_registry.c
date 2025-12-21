/**
 * @file texture_registry.c
 * @brief Texture registry and loading system for PC port
 *
 * Maps game texture symbols to actual texture data loaded from assets.
 * Handles CI4/CI8 indexed textures with TLUT (palette) lookup.
 */

#include "pc/gbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Texture Format Definitions
 * ============================================================================ */

#define TEX_FMT_CI4     0   /* 4-bit color index (16 colors) */
#define TEX_FMT_CI8     1   /* 8-bit color index (256 colors) */
#define TEX_FMT_RGBA16  2   /* 16-bit RGBA5551 */
#define TEX_FMT_RGBA32  3   /* 32-bit RGBA8888 */
#define TEX_FMT_I4      4   /* 4-bit intensity */
#define TEX_FMT_I8      5   /* 8-bit intensity */
#define TEX_FMT_IA4     6   /* 4-bit intensity + alpha */
#define TEX_FMT_IA8     7   /* 8-bit intensity + alpha */
#define TEX_FMT_IA16    8   /* 16-bit intensity + alpha */

/* Maximum registered textures */
#define MAX_TEXTURES    256
#define MAX_PALETTES    64

/* ============================================================================
 * Texture and Palette Structures
 * ============================================================================ */

typedef struct {
    const char* name;           /* Symbol name (e.g., "grass_tex_dummy") */
    u32 dummy_addr;             /* Address of dummy buffer (for matching) */
    u8* data;                   /* Raw texture data */
    int width;
    int height;
    int format;                 /* TEX_FMT_* */
    int palette_idx;            /* Index into palette array (-1 if none) */
    u32 gl_texture_id;          /* OpenGL texture ID (0 if not uploaded) */
} TextureEntry;

typedef struct {
    const char* name;           /* Symbol name (e.g., "bush_pal_dummy") */
    u32 dummy_addr;             /* Address of dummy buffer (for matching) */
    u16* colors;                /* RGBA5551 color data */
    int color_count;            /* 16 for CI4, 256 for CI8 */
    u8 rgba[256][4];            /* Expanded RGBA8888 for fast lookup */
} PaletteEntry;

/* Registry storage */
static TextureEntry g_textures[MAX_TEXTURES];
static int g_texture_count = 0;

static PaletteEntry g_palettes[MAX_PALETTES];
static int g_palette_count = 0;

/* Current active TLUT for rendering */
static int g_active_palette_idx = -1;

/* TMEM simulation (4KB) */
#define TMEM_SIZE 4096
static u8 g_tmem[TMEM_SIZE];
static int g_tmem_texture_idx = -1;  /* Which texture is loaded in TMEM */

/* ============================================================================
 * RGBA5551 to RGBA8888 Conversion
 * ============================================================================ */

static void rgba5551_to_rgba8888(u16 color, u8* r, u8* g, u8* b, u8* a) {
    /* N64 RGBA5551 format: RRRRR GGGGG BBBBB A */
    *r = ((color >> 11) & 0x1F) << 3;  /* 5 bits -> 8 bits */
    *g = ((color >> 6) & 0x1F) << 3;
    *b = ((color >> 1) & 0x1F) << 3;
    *a = (color & 0x01) ? 255 : 0;

    /* Extend low bits for full range */
    *r |= (*r >> 5);
    *g |= (*g >> 5);
    *b |= (*b >> 5);
}

/* ============================================================================
 * Palette Registration and Loading
 * ============================================================================ */

/**
 * Register a palette with the texture system.
 * @param name Symbol name used by the game
 * @param dummy_addr Address of dummy buffer in game code
 * @param colors Array of RGBA5551 colors
 * @param color_count Number of colors (16 for CI4, 256 for CI8)
 * @return Palette index, or -1 on failure
 */
int texture_register_palette(const char* name, u32 dummy_addr,
                             const u16* colors, int color_count) {
    if (g_palette_count >= MAX_PALETTES) {
        printf("[TEX_REG] ERROR: Palette registry full!\n");
        return -1;
    }

    PaletteEntry* pal = &g_palettes[g_palette_count];
    pal->name = name;
    pal->dummy_addr = dummy_addr;
    pal->color_count = color_count;

    /* Allocate and copy color data */
    pal->colors = (u16*)malloc(color_count * sizeof(u16));
    if (!pal->colors) {
        printf("[TEX_REG] ERROR: Failed to allocate palette!\n");
        return -1;
    }
    memcpy(pal->colors, colors, color_count * sizeof(u16));

    /* Pre-expand to RGBA8888 for fast lookup */
    for (int i = 0; i < color_count && i < 256; i++) {
        rgba5551_to_rgba8888(colors[i],
            &pal->rgba[i][0], &pal->rgba[i][1],
            &pal->rgba[i][2], &pal->rgba[i][3]);
    }

    printf("[TEX_REG] Registered palette '%s' with %d colors\n", name, color_count);
    return g_palette_count++;
}

/**
 * Find a palette by its dummy address (truncated pointer matching).
 */
int texture_find_palette_by_addr(u32 addr) {
    for (int i = 0; i < g_palette_count; i++) {
        if (g_palettes[i].dummy_addr == addr) {
            return i;
        }
    }
    return -1;
}

/**
 * Set the active TLUT for subsequent texture operations.
 */
void texture_set_active_tlut(int palette_idx) {
    if (palette_idx >= 0 && palette_idx < g_palette_count) {
        g_active_palette_idx = palette_idx;
    }
}

/* ============================================================================
 * Texture Registration and Loading
 * ============================================================================ */

/**
 * Register a texture with the texture system.
 * @param name Symbol name used by the game
 * @param dummy_addr Address of dummy buffer in game code
 * @param data Raw texture data (CI4/CI8/etc.)
 * @param width Texture width in pixels
 * @param height Texture height in pixels
 * @param format TEX_FMT_* constant
 * @return Texture index, or -1 on failure
 */
int texture_register(const char* name, u32 dummy_addr,
                     const u8* data, int width, int height, int format) {
    if (g_texture_count >= MAX_TEXTURES) {
        printf("[TEX_REG] ERROR: Texture registry full!\n");
        return -1;
    }

    TextureEntry* tex = &g_textures[g_texture_count];
    tex->name = name;
    tex->dummy_addr = dummy_addr;
    tex->width = width;
    tex->height = height;
    tex->format = format;
    tex->palette_idx = -1;
    tex->gl_texture_id = 0;

    /* Calculate data size based on format */
    int data_size;
    switch (format) {
        case TEX_FMT_CI4:
        case TEX_FMT_I4:
        case TEX_FMT_IA4:
            data_size = (width * height) / 2;  /* 4 bits per pixel */
            break;
        case TEX_FMT_CI8:
        case TEX_FMT_I8:
        case TEX_FMT_IA8:
            data_size = width * height;  /* 8 bits per pixel */
            break;
        case TEX_FMT_RGBA16:
        case TEX_FMT_IA16:
            data_size = width * height * 2;  /* 16 bits per pixel */
            break;
        case TEX_FMT_RGBA32:
            data_size = width * height * 4;  /* 32 bits per pixel */
            break;
        default:
            data_size = width * height;
    }

    /* Allocate and copy texture data */
    tex->data = (u8*)malloc(data_size);
    if (!tex->data) {
        printf("[TEX_REG] ERROR: Failed to allocate texture!\n");
        return -1;
    }
    memcpy(tex->data, data, data_size);

    printf("[TEX_REG] Registered texture '%s' %dx%d fmt=%d\n",
           name, width, height, format);
    return g_texture_count++;
}

/**
 * Find a texture by its dummy address.
 */
int texture_find_by_addr(u32 addr) {
    for (int i = 0; i < g_texture_count; i++) {
        if (g_textures[i].dummy_addr == addr) {
            return i;
        }
    }
    return -1;
}

/**
 * Associate a texture with a palette.
 */
void texture_set_palette(int tex_idx, int pal_idx) {
    if (tex_idx >= 0 && tex_idx < g_texture_count) {
        g_textures[tex_idx].palette_idx = pal_idx;
    }
}

/* ============================================================================
 * TMEM Operations (for GBI commands)
 * ============================================================================ */

/**
 * Load a texture into TMEM (simulated).
 * Called by G_LOADBLOCK handler.
 */
void texture_load_to_tmem(int tex_idx) {
    if (tex_idx < 0 || tex_idx >= g_texture_count) return;

    TextureEntry* tex = &g_textures[tex_idx];

    /* Calculate size to copy */
    int size;
    switch (tex->format) {
        case TEX_FMT_CI4:
            size = (tex->width * tex->height) / 2;
            break;
        case TEX_FMT_CI8:
            size = tex->width * tex->height;
            break;
        default:
            size = tex->width * tex->height;
    }

    if (size > TMEM_SIZE) {
        printf("[TEX_REG] WARNING: Texture too large for TMEM!\n");
        size = TMEM_SIZE;
    }

    memcpy(g_tmem, tex->data, size);
    g_tmem_texture_idx = tex_idx;
}

/* ============================================================================
 * OpenGL Texture Upload
 * ============================================================================ */

#include <GL/gl.h>

/**
 * Upload a CI4 texture to OpenGL, expanding with the active palette.
 * @return OpenGL texture ID
 */
GLuint texture_upload_ci4_to_gl(int tex_idx) {
    if (tex_idx < 0 || tex_idx >= g_texture_count) return 0;
    if (g_active_palette_idx < 0) {
        printf("[TEX_REG] ERROR: No active palette for CI4 texture!\n");
        return 0;
    }

    /* Track which texture is current (for get_current_width/height) */
    g_tmem_texture_idx = tex_idx;

    TextureEntry* tex = &g_textures[tex_idx];

    /* Return cached texture if already uploaded */
    if (tex->gl_texture_id != 0) {
        return tex->gl_texture_id;
    }
    PaletteEntry* pal = &g_palettes[g_active_palette_idx];

    /* Allocate RGBA8888 buffer */
    int pixel_count = tex->width * tex->height;
    u8* rgba_data = (u8*)malloc(pixel_count * 4);
    if (!rgba_data) return 0;

    /* Expand CI4 to RGBA8888 using palette */
    int src_idx = 0;
    for (int i = 0; i < pixel_count; i += 2) {
        u8 packed = tex->data[src_idx++];
        int idx0 = (packed >> 4) & 0x0F;  /* High nibble */
        int idx1 = packed & 0x0F;         /* Low nibble */

        /* First pixel */
        rgba_data[i*4 + 0] = pal->rgba[idx0][0];
        rgba_data[i*4 + 1] = pal->rgba[idx0][1];
        rgba_data[i*4 + 2] = pal->rgba[idx0][2];
        rgba_data[i*4 + 3] = pal->rgba[idx0][3];

        /* Second pixel */
        if (i + 1 < pixel_count) {
            rgba_data[(i+1)*4 + 0] = pal->rgba[idx1][0];
            rgba_data[(i+1)*4 + 1] = pal->rgba[idx1][1];
            rgba_data[(i+1)*4 + 2] = pal->rgba[idx1][2];
            rgba_data[(i+1)*4 + 3] = pal->rgba[idx1][3];
        }
    }

    /* Create OpenGL texture */
    GLuint gl_tex;
    glGenTextures(1, &gl_tex);
    glBindTexture(GL_TEXTURE_2D, gl_tex);

    /* Set texture parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* Upload to GPU */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);

    free(rgba_data);

    tex->gl_texture_id = gl_tex;
    printf("[TEX_REG] Uploaded CI4 texture '%s' to GL (id=%u)\n", tex->name, gl_tex);

    return gl_tex;
}

/* ============================================================================
 * Terrain Texture Initialization
 * ============================================================================ */

/* External function from assets_terrain.c */
extern void terrain_assets_register(void);

/**
 * Initialize the terrain textures with real game data.
 */
void texture_init_terrain(void) {
    printf("[TEX_REG] Initializing terrain textures...\n");

    /* Register real terrain textures from assets_terrain.c */
    terrain_assets_register();

    /* Set earth palette as default (used by grass texture) */
    texture_set_active_tlut(0);

    printf("[TEX_REG] Terrain textures initialized\n");
}

/**
 * Get texture info for the GBI interpreter.
 */
int texture_get_current_gl_id(void) {
    if (g_tmem_texture_idx >= 0 && g_tmem_texture_idx < g_texture_count) {
        TextureEntry* tex = &g_textures[g_tmem_texture_idx];
        if (tex->gl_texture_id == 0) {
            /* Upload on first use */
            if (tex->format == TEX_FMT_CI4) {
                texture_upload_ci4_to_gl(g_tmem_texture_idx);
            }
        }
        return tex->gl_texture_id;
    }
    return 0;
}

int texture_get_current_width(void) {
    if (g_tmem_texture_idx >= 0 && g_tmem_texture_idx < g_texture_count) {
        return g_textures[g_tmem_texture_idx].width;
    }
    return 32;  /* default */
}

int texture_get_current_height(void) {
    if (g_tmem_texture_idx >= 0 && g_tmem_texture_idx < g_texture_count) {
        return g_textures[g_tmem_texture_idx].height;
    }
    return 32;  /* default */
}

/**
 * Get the texture segment buffer for segment 9 setup.
 * Returns NULL if textures should be looked up by offset instead.
 * Note: Dolphin GBI commands use segment 9 offsets that map to texture names.
 * The GBI interpreter should intercept these and look up textures from registry.
 */
void* texture_get_buffer(void) {
    /* For now, return NULL - GBI interpreter handles texture lookup by offset */
    /* The offsets in segment 9 are:
     *   0x0000: grass_tex_dummy
     *   0x1000: earth_tex_dummy
     *   0x2000: cliff_tex_dummy
     *   0x3000: bush_a_tex_dummy
     *   0x4000: bush_b_tex_dummy
     *   0x5000: bush_pal_dummy
     *   0x5100: earth_pal_dummy
     *   0x5200: cliff_pal_dummy
     */
    return NULL;
}

/**
 * Look up texture by segment 9 offset.
 * Used by Dolphin GBI command handlers.
 * Maps binary offsets to actual registered texture names.
 */
int texture_find_by_seg9_offset(u32 offset) {
    /* Map offset to registered texture name
     * Binary uses *_dummy names, registry uses actual names */
    const char* name = NULL;
    switch (offset) {
        case 0x0000: name = "grass_tex"; break;      /* grass_tex_dummy -> grass_tex */
        case 0x1000: name = "earth_tex"; break;      /* earth_tex_dummy -> earth_tex */
        case 0x2000: name = "cliff_tex"; break;      /* cliff_tex_dummy -> cliff_tex */
        case 0x3000: name = "bush_a_tex"; break;     /* bush_a_tex_dummy -> bush_a_tex */
        case 0x4000: name = "bush_b_tex"; break;     /* bush_b_tex_dummy -> bush_b_tex */
        default:
            printf("[TEX_REG] Unknown texture offset: 0x%X\n", offset);
            return -1;
    }

    /* Find texture by name */
    for (int i = 0; i < g_texture_count; i++) {
        if (g_textures[i].name && strcmp(g_textures[i].name, name) == 0) {
            return i;
        }
    }
    printf("[TEX_REG] Texture '%s' not found in registry\n", name);
    return -1;
}

/**
 * Look up palette by segment 9 offset.
 * Maps binary offsets to actual registered palette names.
 */
int texture_find_palette_by_seg9_offset(u32 offset) {
    const char* name = NULL;
    switch (offset) {
        case 0x5000: name = "earth_pal"; break;      /* bush_pal_dummy -> earth_pal (shared) */
        case 0x5100: name = "earth_pal"; break;      /* earth_pal_dummy -> earth_pal */
        case 0x5200: name = "cliff_pal"; break;      /* cliff_pal_dummy -> cliff_pal */
        default:
            printf("[TEX_REG] Unknown palette offset: 0x%X\n", offset);
            return -1;
    }

    for (int i = 0; i < g_palette_count; i++) {
        if (g_palettes[i].name && strcmp(g_palettes[i].name, name) == 0) {
            return i;
        }
    }
    printf("[TEX_REG] Palette '%s' not found in registry\n", name);
    return -1;
}

/**
 * Shutdown and free all texture resources.
 */
void texture_shutdown(void) {
    for (int i = 0; i < g_texture_count; i++) {
        if (g_textures[i].data) {
            free(g_textures[i].data);
        }
        if (g_textures[i].gl_texture_id) {
            glDeleteTextures(1, &g_textures[i].gl_texture_id);
        }
    }
    g_texture_count = 0;

    for (int i = 0; i < g_palette_count; i++) {
        if (g_palettes[i].colors) {
            free(g_palettes[i].colors);
        }
    }
    g_palette_count = 0;

    printf("[TEX_REG] Texture registry shutdown\n");
}
