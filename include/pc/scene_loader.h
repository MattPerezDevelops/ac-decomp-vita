/**
 * @file scene_loader.h
 * @brief PC-specific scene data loader with 64-bit pointer support
 */

#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include "m_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the scene loader.
 * Builds runtime scene command tables for all scenes.
 */
void scene_loader_init(void);

/**
 * Get scene data for a given scene ID.
 * Returns the runtime-built Scene_Word_u array.
 *
 * @param scene_id The scene ID (SCENE_FG, SCENE_TITLE_DEMO, etc.)
 * @return Scene command table, or NULL if not implemented
 */
Scene_Word_u* scene_loader_get(int scene_id);

/**
 * Cleanup scene loader resources.
 */
void scene_loader_cleanup(void);

/**
 * Debug: Print scene command table contents.
 */
void scene_loader_debug_print(Scene_Word_u* info);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_LOADER_H */
