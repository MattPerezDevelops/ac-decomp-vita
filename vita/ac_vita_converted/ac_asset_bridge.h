#pragma once

// AC-Decomp to Vita Asset Bridge
// Provides runtime loading of AC-Decomp assets for Vita platform

#include "ac_asset_mapping.h"
#include "ac_runtime_asset_loader.h"
#include <stdint.h>
#include <stdbool.h>

// Asset Bridge Configuration
#define AC_BRIDGE_MAX_LOADED_ASSETS 512
#define AC_BRIDGE_ASSET_CACHE_SIZE (64 * 1024 * 1024)  // 64MB cache

// Asset data types supported
typedef enum {
    AC_ASSET_TYPE_TEXTURE = 0,
    AC_ASSET_TYPE_PALETTE,
    AC_ASSET_TYPE_VERTEX,
    AC_ASSET_TYPE_MODEL,
    AC_ASSET_TYPE_RAW_DATA,
    AC_ASSET_TYPE_COUNT
} ac_asset_type_e;

// Runtime asset handle
typedef struct {
    uint32_t asset_id;
    const char* symbol_name;
    void* data;
    uint32_t size;
    ac_asset_type_e type;
    bool is_loaded;
    uint32_t ref_count;
    const ac_asset_mapping_t* mapping;
} ac_bridge_asset_t;

// Asset bridge initialization and cleanup
int ac_bridge_init(void);
void ac_bridge_cleanup(void);

// Core asset loading functions - replaces AC-Decomp #include "assets/..."
void* ac_get_asset_data(const char* symbol_name);
void* ac_get_asset_data_typed(const char* symbol_name, ac_asset_type_e expected_type);
uint32_t ac_get_asset_size(const char* symbol_name);

// Asset management
bool ac_bridge_preload_symbol(const char* symbol_name);
bool ac_bridge_preload_category(const char* category);
void ac_bridge_unload_symbol(const char* symbol_name);
void ac_bridge_unload_unused(void);

// Memory management
void ac_bridge_add_ref(const char* symbol_name);
void ac_bridge_release_ref(const char* symbol_name);

// Asset information
const ac_asset_mapping_t* ac_bridge_get_mapping(const char* symbol_name);
bool ac_bridge_asset_exists(const char* symbol_name);
ac_asset_type_e ac_bridge_detect_type(const char* symbol_name);

// Cache management
void ac_bridge_clear_cache(void);
uint32_t ac_bridge_get_cache_usage(void);
uint32_t ac_bridge_get_loaded_count(void);

// Debug and statistics
void ac_bridge_print_stats(void);
void ac_bridge_print_loaded_assets(void);
bool ac_bridge_validate_all_mappings(void);

// Utility macros for AC-Decomp source adaptation
#define AC_ASSET_INCLUDE(symbol_name) ac_get_asset_data(#symbol_name)
#define AC_ASSET_INCLUDE_TYPED(symbol_name, type) ac_get_asset_data_typed(#symbol_name, type)

// Type-specific asset getters for convenience
void* ac_get_texture_data(const char* symbol_name);
void* ac_get_palette_data(const char* symbol_name);
void* ac_get_vertex_data(const char* symbol_name);
void* ac_get_model_data(const char* symbol_name);

// Asset streaming and prioritization
typedef enum {
    AC_BRIDGE_PRIORITY_LOW = 0,
    AC_BRIDGE_PRIORITY_NORMAL,
    AC_BRIDGE_PRIORITY_HIGH,
    AC_BRIDGE_PRIORITY_CRITICAL
} ac_bridge_priority_e;

bool ac_bridge_preload_with_priority(const char* symbol_name, ac_bridge_priority_e priority);
void ac_bridge_set_streaming_distance(float distance);
void ac_bridge_update_streaming(float player_x, float player_y, float player_z);

// Error handling
typedef enum {
    AC_BRIDGE_OK = 0,
    AC_BRIDGE_ERROR_NOT_FOUND,
    AC_BRIDGE_ERROR_OUT_OF_MEMORY,
    AC_BRIDGE_ERROR_LOAD_FAILED,
    AC_BRIDGE_ERROR_INVALID_TYPE,
    AC_BRIDGE_ERROR_NOT_INITIALIZED
} ac_bridge_error_e;

ac_bridge_error_e ac_bridge_get_last_error(void);
const char* ac_bridge_error_string(ac_bridge_error_e error);

// Hot reloading for development
#ifdef AC_BRIDGE_ENABLE_HOT_RELOAD
bool ac_bridge_reload_asset(const char* symbol_name);
void ac_bridge_enable_hot_reload(bool enable);
#endif 