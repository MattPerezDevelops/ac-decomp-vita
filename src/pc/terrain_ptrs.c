/**
 * @file terrain_ptrs.c
 * @brief Register terrain texture dummy buffers for address lookup
 *
 * The texture dummy buffers from m_bg_tex.c are referenced by terrain display lists.
 * We register them so the texture registry can intercept loads and substitute
 * real texture data.
 */

#include "pc/gbi.h"
#include <stdio.h>

/* External: GBI interpreter pointer registration */
extern void gbi_register_static_ptr(void* ptr);

/* External: Texture dummy buffers from m_bg_tex.c */
extern u8 grass_tex_dummy[];
extern u8 earth_tex_dummy[];
extern u8 cliff_tex_dummy[];
extern u8 bush_a_tex_dummy[];
extern u8 bush_b_tex_dummy[];
extern u8 bush_pal_dummy[];
extern u8 earth_pal_dummy[];
extern u8 cliff_pal_dummy[];

/**
 * Register all terrain texture dummy buffers.
 * This allows the texture registry to intercept texture loads.
 */
void terrain_register_static_ptrs(void) {
    printf("[TERRAIN_PTRS] Registering texture dummy buffers...\n");

    /* Register texture dummy buffers - these are used by gDPLoadTextureBlock */
    gbi_register_static_ptr(grass_tex_dummy);
    printf("[TERRAIN_PTRS]   grass_tex_dummy = %p (lower32=0x%08X)\n",
           (void*)grass_tex_dummy, (u32)(uintptr_t)grass_tex_dummy);

    gbi_register_static_ptr(earth_tex_dummy);
    gbi_register_static_ptr(cliff_tex_dummy);
    gbi_register_static_ptr(bush_a_tex_dummy);
    gbi_register_static_ptr(bush_b_tex_dummy);

    /* Register palette dummy buffers */
    gbi_register_static_ptr(bush_pal_dummy);
    gbi_register_static_ptr(earth_pal_dummy);
    gbi_register_static_ptr(cliff_pal_dummy);

    printf("[TERRAIN_PTRS] Texture dummies registered\n");
}
