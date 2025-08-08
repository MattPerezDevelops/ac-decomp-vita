#include "ac_runtime_asset_loader.h"
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <stdbool.h>
#include <stdint.h>

// Global state
static ac_loaded_texture_t g_texture_cache[MAX_LOADED_TEXTURES];
static int g_cache_count = 0;
static bool g_runtime_initialized = false;
static bool g_assets_available = false;

int ac_runtime_init(void) {
    printf("🎮 AC Runtime Asset Loader - Initializing...\n");
    
    // Clear texture cache
    memset(g_texture_cache, 0, sizeof(g_texture_cache));
    g_cache_count = 0;
    
    // Check if assets folder exists
    g_assets_available = ac_check_assets_available();
    
    if (g_assets_available) {
        printf("✅ AC Assets found at: %s\n", AC_ASSETS_BASE_PATH);
    } else {
        printf("⚠️  AC Assets not found - using placeholder textures\n");
        printf("   Expected location: %s\n", AC_ASSETS_BASE_PATH);
    }
    
    g_runtime_initialized = true;
    printf("✅ AC Runtime Asset Loader initialized\n");
    
    return g_assets_available ? 1 : 0;
}

void ac_runtime_cleanup(void) {
    if (!g_runtime_initialized) return;
    
    printf("🧹 AC Runtime Asset Loader - Cleanup...\n");
    
    // Free all loaded textures
    ac_clear_texture_cache();
    
    g_runtime_initialized = false;
    printf("✅ AC Runtime Asset Loader cleanup complete\n");
}

GLuint ac_load_texture_runtime(const char* asset_name) {
    if (!g_runtime_initialized) {
        printf("❌ Runtime loader not initialized\n");
        return ac_create_placeholder_texture(asset_name);
    }
    
    // Check if already loaded in cache
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_texture_cache[i].name, asset_name) == 0) {
            if (g_texture_cache[i].is_loaded) {
                return g_texture_cache[i].texture_id;
            }
        }
    }
    
    // Try to load from filesystem
    if (g_assets_available) {
        GLuint texture_id = ac_load_texture_from_file(asset_name);
        if (texture_id != 0) {
            // Add to cache if there's space
            if (g_cache_count < MAX_LOADED_TEXTURES) {
                strncpy(g_texture_cache[g_cache_count].name, asset_name, 63);
                g_texture_cache[g_cache_count].texture_id = texture_id;
                g_texture_cache[g_cache_count].is_loaded = true;
                g_cache_count++;
            }
            return texture_id;
        }
    }
    
    // Fallback to placeholder
    printf("⚠️  Asset '%s' not found, using placeholder\n", asset_name);
    return ac_create_placeholder_texture(asset_name);
}

GLuint ac_load_texture_from_file(const char* asset_name) {
    char file_path[256];
    
    // Try different categories
    const char* categories[] = {"characters", "environment", "items", "ui"};
    const int num_categories = sizeof(categories) / sizeof(categories[0]);
    
    for (int i = 0; i < num_categories; i++) {
        snprintf(file_path, sizeof(file_path), "%stextures/%s/%s.rgba", 
                AC_ASSETS_BASE_PATH, categories[i], asset_name);
        
        FILE* file = fopen(file_path, "rb");
        if (file) {
            // Detect texture dimensions from file size
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            uint32_t width, height;
            // Detect dimensions based on RGBA file size
            if (file_size == 1024) {        // 16x16 RGBA32
                width = 16; height = 16;
            } else if (file_size == 4096) { // 32x32 RGBA32
                width = 32; height = 32;
            } else if (file_size == 2048) { // 32x16 or 16x32 RGBA32
                width = 32; height = 16;    // Assume 32x16
            } else {
                // Default fallback
                width = 32; height = 32;
            }
            
            uint32_t size = width * height * 4;
            
            uint8_t* rgba_data = malloc(size);
            if (!rgba_data) {
                fclose(file);
                continue;
            }
            
            size_t bytes_read = fread(rgba_data, 1, size, file);
            fclose(file);
            
            if (bytes_read == size) {
                // Create OpenGL texture
                GLuint texture_id;
                glGenTextures(1, &texture_id);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                           GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
                
                free(rgba_data);
                
                printf("✅ Loaded texture: %s (%dx%d)\n", asset_name, width, height);
                return texture_id;
            }
            
            free(rgba_data);
        }
    }
    
    return 0; // Not found
}

GLuint ac_create_placeholder_texture(const char* asset_name) {
    // Create a unique colored texture based on asset name
    uint32_t hash = 0;
    for (const char* p = asset_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    uint8_t r = (hash & 0xFF);
    uint8_t g = ((hash >> 8) & 0xFF);
    uint8_t b = ((hash >> 16) & 0xFF);
    uint8_t a = 255;
    
    return ac_create_colored_texture(r, g, b, a);
}

GLuint ac_create_colored_texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const uint32_t width = 32, height = 32;
    const uint32_t size = width * height * 4;
    
    uint8_t* rgba_data = malloc(size);
    if (!rgba_data) return 0;
    
    // Fill with solid color
    for (uint32_t i = 0; i < width * height; i++) {
        rgba_data[i * 4 + 0] = r;
        rgba_data[i * 4 + 1] = g;
        rgba_data[i * 4 + 2] = b;
        rgba_data[i * 4 + 3] = a;
    }
    
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
               GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
    
    free(rgba_data);
    return texture_id;
}

bool ac_check_assets_available(void) {
    char manifest_path[256];
    snprintf(manifest_path, sizeof(manifest_path), "%smanifest.txt", AC_ASSETS_BASE_PATH);
    
    FILE* file = fopen(manifest_path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    
    return false;
}

void ac_clear_texture_cache(void) {
    for (int i = 0; i < g_cache_count; i++) {
        if (g_texture_cache[i].is_loaded) {
            glDeleteTextures(1, &g_texture_cache[i].texture_id);
        }
    }
    
    memset(g_texture_cache, 0, sizeof(g_texture_cache));
    g_cache_count = 0;
}

bool ac_asset_exists(const char* asset_name) {
    // Simple check - try to load and see if we get a placeholder
    GLuint test_texture = ac_load_texture_runtime(asset_name);
    
    if (test_texture != 0) {
        // Check if it's in our cache
        for (int i = 0; i < g_cache_count; i++) {
            if (strcmp(g_texture_cache[i].name, asset_name) == 0) {
                return true;
            }
        }
    }
    
    return false;
}

int ac_get_cache_usage(void) {
    return g_cache_count;
}

const char* ac_get_assets_version(void) {
    return "1.0.0";
} 