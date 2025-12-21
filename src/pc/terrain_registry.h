#ifndef TERRAIN_REGISTRY_H
#define TERRAIN_REGISTRY_H

/**
 * Terrain Registry
 *
 * Registers terrain display lists and vertex arrays with the pointer
 * registry for 64-bit pointer recovery.
 *
 * Call terrain_registry_init() after ptr_registry_init() at startup.
 */

void terrain_registry_init(void);

#endif /* TERRAIN_REGISTRY_H */
