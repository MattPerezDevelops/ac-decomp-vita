#include "ac_runtime_asset_loader_mock.h"
#include <stdio.h>
#include <string.h>

// Mock implementation for testing
static bool g_mock_initialized = false;

int ac_runtime_init(void) {
    printf("📦 Mock Runtime Asset Loader - Initializing...\n");
    g_mock_initialized = true;
    return 1;
}

void ac_runtime_cleanup(void) {
    printf("📦 Mock Runtime Asset Loader - Cleanup...\n");
    g_mock_initialized = false;
}

GLuint ac_load_texture_runtime(const char* asset_name) {
    if (!g_mock_initialized) return 0;
    
    printf("📦 Mock loading texture: %s\n", asset_name);
    return 1; // Mock texture ID
}

GLuint ac_load_texture_from_file(const char* asset_name) {
    printf("📦 Mock loading texture from file: %s\n", asset_name);
    return 1; // Mock texture ID
}

bool ac_preload_category(const char* category) {
    printf("📦 Mock preloading category: %s\n", category);
    return true;
}

void ac_unload_unused_textures(void) {
    printf("📦 Mock unloading unused textures\n");
}

bool ac_asset_exists(const char* asset_name) {
    // Mock - assume some common assets exist
    return strstr(asset_name, "tex") != NULL || strstr(asset_name, "pal") != NULL;
}

ac_runtime_asset_t* ac_get_asset_info(const char* asset_name) {
    return NULL; // Mock - not implemented
}

void ac_clear_texture_cache(void) {
    printf("📦 Mock clearing texture cache\n");
}

int ac_get_cache_usage(void) {
    return 0; // Mock usage
}

GLuint ac_create_placeholder_texture(const char* asset_name) {
    printf("📦 Mock creating placeholder for: %s\n", asset_name);
    return 1; // Mock texture ID
}

GLuint ac_create_colored_texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return 1; // Mock texture ID
}

bool ac_check_assets_available(void) {
    return false; // Mock - no real assets available
}

const char* ac_get_assets_version(void) {
    return "Mock v1.0";
} 