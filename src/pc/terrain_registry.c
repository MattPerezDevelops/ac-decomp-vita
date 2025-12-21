#include "ptr_registry.h"
#include <stdio.h>

/**
 * Terrain Registry
 *
 * Registers terrain-related pointers with the pointer registry.
 *
 * NOTE: The original terrain display lists (data_bgd[]) cannot be compiled
 * on 64-bit because gsSPVertex() uses (u32)(uintptr_t)ptr in static
 * initializers, which is not a constant expression in C.
 *
 * The binary terrain approach (terrain_loader.c) works around this by
 * storing display lists in a format that can be patched at load time.
 *
 * This registry is kept for future use with other pointer recovery needs,
 * such as actor display lists or dynamically loaded resources.
 */

void terrain_registry_init(void) {
    printf("[TERRAIN_REG] Terrain registry initialized\n");
    printf("[TERRAIN_REG] Note: Using binary terrain approach for display lists\n");
    printf("[TERRAIN_REG] Pointer registry has %d entries\n", ptr_registry_count());
}
