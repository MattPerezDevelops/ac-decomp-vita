#include "ac_asset_bridge_mock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Global state
static ac_bridge_asset_t g_loaded_assets[AC_BRIDGE_MAX_LOADED_ASSETS];
static uint32_t g_loaded_count = 0;
static bool g_bridge_initialized = false;
static ac_bridge_error_e g_last_error = AC_BRIDGE_OK;
static uint32_t g_cache_usage = 0;
static uint32_t g_next_asset_id = 1;

// Private functions
static ac_bridge_asset_t* find_loaded_asset(const char* symbol_name);
static ac_bridge_asset_t* allocate_asset_slot(void);
static void free_asset_slot(ac_bridge_asset_t* asset);
static ac_asset_type_e detect_asset_type_from_name(const char* symbol_name);
static bool load_asset_data(ac_bridge_asset_t* asset);

int ac_bridge_init(void) {
    if (g_bridge_initialized) {
        return 1; // Already initialized
    }
    
    printf("🌉 AC Asset Bridge - Initializing...\n");
    
    // Clear asset slots
    memset(g_loaded_assets, 0, sizeof(g_loaded_assets));
    g_loaded_count = 0;
    g_cache_usage = 0;
    g_next_asset_id = 1;
    g_last_error = AC_BRIDGE_OK;
    
    // Initialize runtime asset loader
    int loader_result = ac_runtime_init();
    if (loader_result <= 0) {
        printf("⚠️  Asset bridge: Runtime loader initialization failed\n");
        // Continue anyway - we can still work with placeholder assets
    }
    
    // Validate asset mapping
    if (!ac_validate_asset_mapping()) {
        printf("❌ Asset bridge: Asset mapping validation failed\n");
        g_last_error = AC_BRIDGE_ERROR_NOT_FOUND;
        return 0;
    }
    
    g_bridge_initialized = true;
    printf("✅ AC Asset Bridge initialized\n");
    printf("   Mapped assets: %d\n", AC_ASSET_MAPPING_COUNT);
    printf("   Cache size: %u MB\n", AC_BRIDGE_ASSET_CACHE_SIZE / (1024 * 1024));
    
    return 1;
}

void ac_bridge_cleanup(void) {
    if (!g_bridge_initialized) return;
    
    printf("🧹 AC Asset Bridge - Cleanup...\n");
    
    // Free all loaded assets
    for (uint32_t i = 0; i < g_loaded_count; i++) {
        if (g_loaded_assets[i].data) {
            free(g_loaded_assets[i].data);
        }
    }
    
    // Cleanup runtime loader
    ac_runtime_cleanup();
    
    memset(g_loaded_assets, 0, sizeof(g_loaded_assets));
    g_loaded_count = 0;
    g_cache_usage = 0;
    g_bridge_initialized = false;
    
    printf("✅ AC Asset Bridge cleanup complete\n");
}

void* ac_get_asset_data(const char* symbol_name) {
    if (!g_bridge_initialized) {
        printf("❌ Asset bridge not initialized\n");
        g_last_error = AC_BRIDGE_ERROR_NOT_INITIALIZED;
        return NULL;
    }
    
    if (!symbol_name) {
        g_last_error = AC_BRIDGE_ERROR_NOT_FOUND;
        return NULL;
    }
    
    // Check if already loaded
    ac_bridge_asset_t* existing = find_loaded_asset(symbol_name);
    if (existing && existing->is_loaded) {
        existing->ref_count++;
        return existing->data;
    }
    
    // Find mapping for this symbol
    const ac_asset_mapping_t* mapping = ac_find_asset_mapping(symbol_name);
    if (!mapping) {
        printf("⚠️  Asset '%s' not found in mapping table\n", symbol_name);
        g_last_error = AC_BRIDGE_ERROR_NOT_FOUND;
        return NULL;
    }
    
    // Allocate new asset slot
    ac_bridge_asset_t* asset = allocate_asset_slot();
    if (!asset) {
        printf("❌ No free asset slots for '%s'\n", symbol_name);
        g_last_error = AC_BRIDGE_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    // Initialize asset
    asset->asset_id = g_next_asset_id++;
    asset->symbol_name = symbol_name;
    asset->mapping = mapping;
    asset->type = detect_asset_type_from_name(symbol_name);
    asset->ref_count = 1;
    asset->is_loaded = false;
    
    // Load asset data
    if (!load_asset_data(asset)) {
        printf("❌ Failed to load asset data for '%s'\n", symbol_name);
        free_asset_slot(asset);
        g_last_error = AC_BRIDGE_ERROR_LOAD_FAILED;
        return NULL;
    }
    
    printf("✅ Loaded asset '%s' (%u bytes)\n", symbol_name, asset->size);
    g_last_error = AC_BRIDGE_OK;
    return asset->data;
}

void* ac_get_asset_data_typed(const char* symbol_name, ac_asset_type_e expected_type) {
    void* data = ac_get_asset_data(symbol_name);
    if (!data) return NULL;
    
    ac_bridge_asset_t* asset = find_loaded_asset(symbol_name);
    if (asset && asset->type != expected_type) {
        printf("⚠️  Asset '%s' type mismatch: expected %d, got %d\n", 
               symbol_name, expected_type, asset->type);
        g_last_error = AC_BRIDGE_ERROR_INVALID_TYPE;
    }
    
    return data;
}

uint32_t ac_get_asset_size(const char* symbol_name) {
    ac_bridge_asset_t* asset = find_loaded_asset(symbol_name);
    if (asset && asset->is_loaded) {
        return asset->size;
    }
    
    const ac_asset_mapping_t* mapping = ac_find_asset_mapping(symbol_name);
    if (!mapping) {
        g_last_error = AC_BRIDGE_ERROR_NOT_FOUND;
        return 0;
    }
    
    // For unloaded assets, we'd need to check file size
    // For now, return 0 to indicate unknown
    return 0;
}

bool ac_bridge_preload_symbol(const char* symbol_name) {
    void* data = ac_get_asset_data(symbol_name);
    return data != NULL;
}

bool ac_bridge_preload_category(const char* category) {
    const ac_asset_mapping_t* results[256];
    int count = ac_get_assets_by_category(category, results, 256);
    
    printf("🔄 Preloading %d assets from category '%s'...\n", count, category);
    
    int loaded = 0;
    for (int i = 0; i < count; i++) {
        if (ac_bridge_preload_symbol(results[i]->symbol_name)) {
            loaded++;
        }
    }
    
    printf("✅ Preloaded %d/%d assets from category '%s'\n", loaded, count, category);
    return loaded > 0;
}

void ac_bridge_unload_symbol(const char* symbol_name) {
    ac_bridge_asset_t* asset = find_loaded_asset(symbol_name);
    if (!asset) return;
    
    asset->ref_count--;
    if (asset->ref_count <= 0) {
        free_asset_slot(asset);
        printf("🗑️  Unloaded asset '%s'\n", symbol_name);
    }
}

void ac_bridge_unload_unused(void) {
    uint32_t unloaded = 0;
    
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        ac_bridge_asset_t* asset = &g_loaded_assets[i];
        if (asset->is_loaded && asset->ref_count <= 0) {
            free_asset_slot(asset);
            unloaded++;
        }
    }
    
    if (unloaded > 0) {
        printf("🗑️  Unloaded %u unused assets\n", unloaded);
    }
}

// Convenience functions for typed access
void* ac_get_texture_data(const char* symbol_name) {
    return ac_get_asset_data_typed(symbol_name, AC_ASSET_TYPE_TEXTURE);
}

void* ac_get_palette_data(const char* symbol_name) {
    return ac_get_asset_data_typed(symbol_name, AC_ASSET_TYPE_PALETTE);
}

void* ac_get_vertex_data(const char* symbol_name) {
    return ac_get_asset_data_typed(symbol_name, AC_ASSET_TYPE_VERTEX);
}

void* ac_get_model_data(const char* symbol_name) {
    return ac_get_asset_data_typed(symbol_name, AC_ASSET_TYPE_MODEL);
}

// Asset information functions
const ac_asset_mapping_t* ac_bridge_get_mapping(const char* symbol_name) {
    return ac_find_asset_mapping(symbol_name);
}

bool ac_bridge_asset_exists(const char* symbol_name) {
    return ac_find_asset_mapping(symbol_name) != NULL;
}

ac_asset_type_e ac_bridge_detect_type(const char* symbol_name) {
    return detect_asset_type_from_name(symbol_name);
}

// Cache management
void ac_bridge_clear_cache(void) {
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        if (g_loaded_assets[i].is_loaded) {
            free_asset_slot(&g_loaded_assets[i]);
        }
    }
    printf("🗑️  Cleared asset cache\n");
}

uint32_t ac_bridge_get_cache_usage(void) {
    return g_cache_usage;
}

uint32_t ac_bridge_get_loaded_count(void) {
    return g_loaded_count;
}

// Debug functions
void ac_bridge_print_stats(void) {
    printf("📊 AC Asset Bridge Statistics\n");
    printf("=============================\n");
    printf("Initialized: %s\n", g_bridge_initialized ? "Yes" : "No");
    printf("Loaded assets: %u / %u\n", g_loaded_count, AC_BRIDGE_MAX_LOADED_ASSETS);
    printf("Cache usage: %.2f MB / %.2f MB\n", 
           g_cache_usage / (1024.0f * 1024.0f),
           AC_BRIDGE_ASSET_CACHE_SIZE / (1024.0f * 1024.0f));
    printf("Last error: %d\n", g_last_error);
}

void ac_bridge_print_loaded_assets(void) {
    printf("📋 Loaded Assets (%u total):\n", g_loaded_count);
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        ac_bridge_asset_t* asset = &g_loaded_assets[i];
        if (asset->is_loaded) {
            printf("  %s: %u bytes (refs: %u, type: %d)\n", 
                   asset->symbol_name, asset->size, asset->ref_count, asset->type);
        }
    }
}

// Error handling
ac_bridge_error_e ac_bridge_get_last_error(void) {
    return g_last_error;
}

const char* ac_bridge_error_string(ac_bridge_error_e error) {
    switch (error) {
        case AC_BRIDGE_OK: return "Success";
        case AC_BRIDGE_ERROR_NOT_FOUND: return "Asset not found";
        case AC_BRIDGE_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case AC_BRIDGE_ERROR_LOAD_FAILED: return "Load failed";
        case AC_BRIDGE_ERROR_INVALID_TYPE: return "Invalid type";
        case AC_BRIDGE_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}

// Private implementation functions
static ac_bridge_asset_t* find_loaded_asset(const char* symbol_name) {
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        ac_bridge_asset_t* asset = &g_loaded_assets[i];
        if (asset->is_loaded && asset->symbol_name && 
            strcmp(asset->symbol_name, symbol_name) == 0) {
            return asset;
        }
    }
    return NULL;
}

static ac_bridge_asset_t* allocate_asset_slot(void) {
    // First, try to find an empty slot
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        if (!g_loaded_assets[i].is_loaded) {
            g_loaded_count++;
            return &g_loaded_assets[i];
        }
    }
    
    // No free slots - try to evict unused assets
    for (uint32_t i = 0; i < AC_BRIDGE_MAX_LOADED_ASSETS; i++) {
        if (g_loaded_assets[i].ref_count == 0) {
            free_asset_slot(&g_loaded_assets[i]);
            g_loaded_count++;
            return &g_loaded_assets[i];
        }
    }
    
    return NULL; // No slots available
}

static void free_asset_slot(ac_bridge_asset_t* asset) {
    if (!asset || !asset->is_loaded) return;
    
    if (asset->data) {
        g_cache_usage -= asset->size;
        free(asset->data);
    }
    
    memset(asset, 0, sizeof(ac_bridge_asset_t));
    g_loaded_count--;
}

static ac_asset_type_e detect_asset_type_from_name(const char* symbol_name) {
    if (!symbol_name) return AC_ASSET_TYPE_RAW_DATA;
    
    // Check for palette indicators
    if (strstr(symbol_name, "_pal") || strstr(symbol_name, "_palette")) {
        return AC_ASSET_TYPE_PALETTE;
    }
    
    // Check for texture indicators
    if (strstr(symbol_name, "_tex") || strstr(symbol_name, "_texture")) {
        return AC_ASSET_TYPE_TEXTURE;
    }
    
    // Check for vertex data
    if (strstr(symbol_name, "_v") || strstr(symbol_name, "_vtx") || strstr(symbol_name, "_vertex")) {
        return AC_ASSET_TYPE_VERTEX;
    }
    
    // Check for model data
    if (strstr(symbol_name, "_model") || strstr(symbol_name, "_mdl") || strstr(symbol_name, "_gfx")) {
        return AC_ASSET_TYPE_MODEL;
    }
    
    return AC_ASSET_TYPE_RAW_DATA;
}

static bool load_asset_data(ac_bridge_asset_t* asset) {
    if (!asset || !asset->mapping) return false;
    
    // Try to load using runtime asset loader
    const char* vita_file = asset->mapping->vita_file;
    
    // For now, create placeholder data based on the asset name
    // In a real implementation, this would load the actual .rgba file
    const char* asset_name = asset->mapping->asset_name;
    
    // Estimate size based on asset type
    uint32_t estimated_size;
    switch (asset->type) {
        case AC_ASSET_TYPE_TEXTURE:
            estimated_size = 64 * 64 * 4; // Assume 64x64 RGBA
            break;
        case AC_ASSET_TYPE_PALETTE:
            estimated_size = 256 * 2; // Assume 256 color 16-bit palette
            break;
        case AC_ASSET_TYPE_VERTEX:
            estimated_size = 1024; // Small vertex buffer
            break;
        case AC_ASSET_TYPE_MODEL:
            estimated_size = 4096; // Model data
            break;
        default:
            estimated_size = 512; // Default small size
            break;
    }
    
    // Check cache limits
    if (g_cache_usage + estimated_size > AC_BRIDGE_ASSET_CACHE_SIZE) {
        printf("⚠️  Cache full, evicting assets...\n");
        ac_bridge_unload_unused();
        
        if (g_cache_usage + estimated_size > AC_BRIDGE_ASSET_CACHE_SIZE) {
            printf("❌ Not enough cache space for asset '%s'\n", asset->symbol_name);
            return false;
        }
    }
    
    // Allocate memory for asset data
    asset->data = malloc(estimated_size);
    if (!asset->data) {
        printf("❌ Failed to allocate %u bytes for asset '%s'\n", 
               estimated_size, asset->symbol_name);
        return false;
    }
    
    // TODO: Load actual asset data from .rgba file
    // For now, fill with placeholder data
    memset(asset->data, 0xAC, estimated_size); // AC = Animal Crossing :)
    
    asset->size = estimated_size;
    asset->is_loaded = true;
    g_cache_usage += estimated_size;
    
    return true;
} 