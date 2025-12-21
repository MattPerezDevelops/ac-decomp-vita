/**
 * @file logo_assets.c
 * @brief Real logo background assets for PC port
 *
 * Contains the actual texture and vertex data for the Animal Crossing logo
 * background, with display lists built at runtime to avoid 64-bit pointer issues.
 *
 * Assets extracted from: build/GAFE01_00/include/assets/logo_us_back_*.inc
 */

#include <stdio.h>
#include "types.h"
#include "libforest/gbi_extensions.h"
#include "PR/gbi.h"
#include "pc/ptr_registry.h"

/* Texture dimensions: 64x128 I4 = 4096 bytes each */
#define LOGO_TEX_WIDTH  64
#define LOGO_TEX_HEIGHT 128
#define LOGO_TEX_SIZE   4096

/* Vertex data from logo_us_back_v.inc */
static Vtx logo_us_back_v[] = {
    /* Panel D (rightmost) - indices 0-3 */
    {.v = {{5472, -6225, -1000}, 1, {32, 4096}, {0, 0, 120, 255}}},
    {.v = {{11381, -6225, -1000}, 1, {2048, 4096}, {0, 0, 120, 255}}},
    {.v = {{11381, 5975, -1000}, 1, {2048, 0}, {0, 0, 120, 255}}},
    {.v = {{5472, 5975, -1000}, 1, {32, 0}, {0, 0, 120, 255}}},
    /* Panel C - indices 4-7 */
    {.v = {{-438, -6225, -1000}, 1, {32, 4096}, {255, 255, 255, 255}}},
    {.v = {{5472, -6225, -1000}, 1, {2016, 4096}, {255, 255, 255, 255}}},
    {.v = {{5472, 5975, -1000}, 1, {2016, 0}, {255, 255, 255, 255}}},
    {.v = {{-438, 5975, -1000}, 1, {32, 0}, {255, 255, 255, 255}}},
    /* Panel A - indices 8-11 */
    {.v = {{-12256, -6225, -1000}, 1, {0, 4096}, {255, 255, 255, 255}}},
    {.v = {{-6347, -6225, -1000}, 1, {2016, 4096}, {255, 255, 255, 255}}},
    {.v = {{-6347, 5975, -1000}, 1, {2016, 0}, {255, 255, 255, 255}}},
    {.v = {{-12256, 5975, -1000}, 1, {0, 0}, {255, 255, 255, 255}}},
    /* Panel B - indices 12-15 */
    {.v = {{-6347, -6225, -1000}, 1, {32, 4096}, {255, 255, 255, 255}}},
    {.v = {{-438, -6225, -1000}, 1, {2016, 4096}, {255, 255, 255, 255}}},
    {.v = {{-438, 5975, -1000}, 1, {2016, 0}, {255, 255, 255, 255}}},
    {.v = {{-6347, 5975, -1000}, 1, {32, 0}, {255, 255, 255, 255}}},
};

/* Texture data from logo_us_back_*_tex_4i4_txt.inc */
/* Panel 1 texture (backA) */
static u8 logo_us_back_1_tex[LOGO_TEX_SIZE] ATTRIBUTE_ALIGN(32) = {
#include "../../build/GAFE01_00/include/assets/logo_us_back_1_tex_4i4_txt.inc"
};

/* Panel 2 texture (backB) */
static u8 logo_us_back_2_tex[LOGO_TEX_SIZE] ATTRIBUTE_ALIGN(32) = {
#include "../../build/GAFE01_00/include/assets/logo_us_back_2_tex_4i4_txt.inc"
};

/* Panel 3 texture (backC) */
static u8 logo_us_back_3_tex[LOGO_TEX_SIZE] ATTRIBUTE_ALIGN(32) = {
#include "../../build/GAFE01_00/include/assets/logo_us_back_3_tex_4i4_txt.inc"
};

/* Panel 4 texture (backD) */
static u8 logo_us_back_4_tex[LOGO_TEX_SIZE] ATTRIBUTE_ALIGN(32) = {
#include "../../build/GAFE01_00/include/assets/logo_us_back_4_tex_4i4_txt.inc"
};

/* Runtime-built display lists - MUST be arrays to match extern declarations
 * The actor declares: extern Gfx logo_us_backA_model[];
 * If we define as pointer, the symbol lookup will treat our pointer VALUE
 * as the first Gfx command, causing crashes or random behavior.
 */
Gfx logo_us_backA_model[8];
Gfx logo_us_backB_model[8];
Gfx logo_us_backC_model[8];
Gfx logo_us_backD_model[8];

/* Helper to emit one logo panel display list */
static Gfx* build_logo_panel_dl(Gfx* dl, u8* tex, Vtx* vtx_start) {
    Gfx* start = dl;

    /* Set texture (64x128 I4) */
    dl->words.w0 = (0xFD << 24) | (G_IM_FMT_I << 21) | (G_IM_SIZ_4b << 19) |
                   ((LOGO_TEX_WIDTH - 1) << 8) | (LOGO_TEX_HEIGHT - 1);
    dl->words.w1 = (u32)(uintptr_t)tex;
    dl++;

    /* Set tile parameters */
    dl->words.w0 = (0xF5 << 24) | (G_IM_FMT_I << 21) | (G_IM_SIZ_4b << 19);
    dl->words.w1 = 0;  /* Default tile settings */
    dl++;

    /* Load vertices */
    dl->words.w0 = (G_VTX << 24) | (4 << 12) | (4 << 1);
    dl->words.w1 = (u32)(uintptr_t)vtx_start;
    dl++;

    /* Draw triangles (2 tris for quad) */
    dl->words.w0 = (G_TRI1 << 24) | (0 << 17) | (1 << 9) | (2 << 1);
    dl->words.w1 = 0;
    dl++;

    dl->words.w0 = (G_TRI1 << 24) | (0 << 17) | (2 << 9) | (3 << 1);
    dl->words.w1 = 0;
    dl++;

    /* End display list */
    dl->words.w0 = (G_ENDDL << 24);
    dl->words.w1 = 0;
    dl++;

    return dl;
}

/**
 * Initialize logo assets - builds display lists at runtime
 * Called during PC port initialization
 */
void logo_assets_init(void) {
    printf("[LOGO] Initializing logo background assets...\n");

    /* Register all pointers used in display lists so they can be recovered
     * when the GBI interpreter executes the commands with truncated w1 values */

    /* Register display lists (so gSPDisplayList can find them) */
    ptr_registry_add(logo_us_backA_model, "logo_dlA");
    ptr_registry_add(logo_us_backB_model, "logo_dlB");
    ptr_registry_add(logo_us_backC_model, "logo_dlC");
    ptr_registry_add(logo_us_backD_model, "logo_dlD");

    /* Register textures (so G_SETTIMG can find them) */
    ptr_registry_add(logo_us_back_1_tex, "logo_tex1");
    ptr_registry_add(logo_us_back_2_tex, "logo_tex2");
    ptr_registry_add(logo_us_back_3_tex, "logo_tex3");
    ptr_registry_add(logo_us_back_4_tex, "logo_tex4");

    /* Register vertex arrays (so G_VTX can find them) */
    ptr_registry_add(&logo_us_back_v[0], "logo_vtx_D");
    ptr_registry_add(&logo_us_back_v[4], "logo_vtx_C");
    ptr_registry_add(&logo_us_back_v[8], "logo_vtx_A");
    ptr_registry_add(&logo_us_back_v[12], "logo_vtx_B");

    /* Build display lists directly into the global arrays.
     * These arrays ARE the display lists - when the actor calls
     * gSPDisplayList(gfx++, logo_us_backA_model), it jumps here.
     */

    /* Panel A uses vertices 8-11 and texture 1 */
    build_logo_panel_dl(logo_us_backA_model, logo_us_back_1_tex, &logo_us_back_v[8]);

    /* Panel B uses vertices 12-15 and texture 2 */
    build_logo_panel_dl(logo_us_backB_model, logo_us_back_2_tex, &logo_us_back_v[12]);

    /* Panel C uses vertices 4-7 and texture 3 */
    build_logo_panel_dl(logo_us_backC_model, logo_us_back_3_tex, &logo_us_back_v[4]);

    /* Panel D uses vertices 0-3 and texture 4 */
    build_logo_panel_dl(logo_us_backD_model, logo_us_back_4_tex, &logo_us_back_v[0]);

    printf("[LOGO] Logo assets initialized (arrays):\n");
    printf("[LOGO]   A=%p B=%p C=%p D=%p\n",
           (void*)logo_us_backA_model, (void*)logo_us_backB_model,
           (void*)logo_us_backC_model, (void*)logo_us_backD_model);
    printf("[LOGO] Logo vertices initialized at %p, texture at %p\n",
           (void*)logo_us_back_v, (void*)logo_us_back_1_tex);
}
