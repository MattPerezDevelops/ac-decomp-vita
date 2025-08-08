#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <vitaGL.h>
#include "ac_runtime_asset_loader.h"

// Enhanced texture info structure
typedef struct {
    GLuint texture_id;
    int width;
    int height;
    char name[64];
    bool loaded;
} enhanced_texture_info_t;

// Global state
static enhanced_texture_info_t g_current_texture = {0};
// Global texture debug state
static bool g_texture_debug = true;  // Enable debug by default

// Enhanced texture loading with proper GameCube format support
GLuint enhanced_load_texture(const char* asset_name) {
    printf("🎮 Enhanced loading: %s\n", asset_name);
    
    // Try different categories like the working asset loader
    const char* categories[] = {"characters", "environment", "items", "ui"};
    const int num_categories = sizeof(categories) / sizeof(categories[0]);
    
    FILE* file = NULL;
    char file_path[256];
    
    // DEBUG: Show what we're looking for
    printf("🔍 DEBUG: Searching for asset '%s' in %d categories...\n", asset_name, num_categories);
    
    // Search across all categories AND check multiple possible base paths
    const char* base_paths[] = {
        "ux0:data/AnimalCrossing/assets/textures/",     // Current expected path
        "ux0:data/AnimalCrossingVita/assets/textures/", // Alternative path
        "ux0:data/AnimalCrossing/assets/",              // Flat structure
        "ux0:data/AnimalCrossingVita/assets/"           // Alternative flat
    };
    const int num_base_paths = sizeof(base_paths) / sizeof(base_paths[0]);
    
    for (int b = 0; b < num_base_paths; b++) {
        for (int i = 0; i < num_categories; i++) {
            snprintf(file_path, sizeof(file_path), 
                     "%s%s/%s.rgba", 
                     base_paths[b], categories[i], asset_name);
            
            printf("🔍 DEBUG: Trying path: %s\n", file_path);
            
            file = fopen(file_path, "rb");
            if (file) {
                printf("✅ Found texture at: %s\n", file_path);
                goto file_found;
            } else {
                printf("❌ Not found: %s\n", file_path);
            }
        }
        
        // Also try flat structure (no category subfolder)
        snprintf(file_path, sizeof(file_path), 
                 "%s%s.rgba", 
                 base_paths[b], asset_name);
        
        printf("🔍 DEBUG: Trying flat path: %s\n", file_path);
        
        file = fopen(file_path, "rb");
        if (file) {
            printf("✅ Found texture at flat path: %s\n", file_path);
            goto file_found;
        } else {
            printf("❌ Not found flat: %s\n", file_path);
        }
    }
    
    file_found:
    
    if (!file) {
        printf("❌ Failed to find texture in any category: %s\n", asset_name);
        printf("🔍 DEBUG: Checked %d categories, all failed\n", num_categories);
        
        // DEBUG: Try to check if the base directory exists
        FILE* test_dir = fopen("ux0:data/AnimalCrossing/assets/", "rb");
        if (test_dir) {
            fclose(test_dir);
            printf("✅ Base asset directory exists\n");
        } else {
            printf("❌ Base asset directory missing: ux0:data/AnimalCrossing/assets/\n");
        }
        
        return 0;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Enhanced dimension detection for GameCube formats
    int width, height;
    const char* format_name;
    
    // Based on comprehensive pipeline analysis of 16,361 AC files
    // Top sizes: 256 bytes (4,899 files), 512 bytes (1,811 files), 128 bytes (2,069 files)
    
    if (file_size == 256) {         // 8x8 RGBA32 (tiny icons) - from CI4 32x16 conversion
        width = 8; height = 8; format_name = "Icon 8x8";
    } else if (file_size == 512) {  // 16x8 RGBA32 (small strips) - from CI4 32x32 conversion  
        width = 16; height = 8; format_name = "Strip 16x8";
    } else if (file_size == 1024) { // 16x16 RGBA32 (square) - from CI8 32x16 conversion
        width = 16; height = 16; format_name = "Square 16x16";
    } else if (file_size == 2048) { // 32x16 RGBA32 (AC character textures!) - from CI4 32x16
        width = 32; height = 16; format_name = "AC Character 32x16";
    } else if (file_size == 4096) { // 32x32 RGBA32 (standard) - from CI8 32x32 conversion
        width = 32; height = 32; format_name = "Standard 32x32";
    } else if (file_size == 8192) { // 64x32 RGBA32 (large) - from CI8 64x32 conversion
        width = 64; height = 32; format_name = "Large 64x32";
    } else if (file_size == 16384) { // 64x64 RGBA32 (very large) - from RGB5A3 conversion
        width = 64; height = 64; format_name = "XLarge 64x64";
    } else if (file_size == 128) {  // 8x4 RGBA32 (very small) - from I4/I8 conversion
        width = 8; height = 4; format_name = "Tiny 8x4";
    } else if (file_size == 3072) { // 24x32 RGBA32 (unusual size from analysis)
        width = 24; height = 32; format_name = "Unusual 24x32";
    } else if (file_size == 6144) { // 32x48 RGBA32 (large unusual)
        width = 32; height = 48; format_name = "Large 32x48";
    } else if (file_size == 12288) { // 64x48 RGBA32 (very large unusual)
        width = 64; height = 48; format_name = "XL 64x48";
    } else {
        // Calculate dimensions from file size
        int total_pixels = file_size / 4;
        width = 32; height = total_pixels / 32; // Default to 32-wide
        if (height < 1) { width = 16; height = total_pixels / 16; }
        if (height < 1) { width = 8; height = total_pixels / 8; }
        format_name = "Calculated";
    }
    
    printf("📊 Enhanced detection: %dx%d %s (%ld bytes)\n", width, height, format_name, file_size);
    
    // Allocate and read data
    uint32_t expected_size = width * height * 4;
    uint8_t* rgba_data = malloc(expected_size);
    if (!rgba_data) {
        fclose(file);
        printf("❌ Memory allocation failed for %u bytes\n", expected_size);
        return 0;
    }
    
    size_t bytes_read = fread(rgba_data, 1, expected_size, file);
    fclose(file);
    
    if (bytes_read != expected_size) {
        printf("❌ Read %zu bytes, expected %u\n", bytes_read, expected_size);
        free(rgba_data);
        return 0;
    }
    
    // Create OpenGL texture with GameCube-optimized settings
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // GameCube-optimized texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Pixel-perfect for small textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // No blurring
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevent edge artifacts
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // VitaGL handles RGBA32 alignment automatically
    // No need for glPixelStorei in VitaGL
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba_data);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("❌ OpenGL error creating texture: 0x%x\n", error);
        glDeleteTextures(1, &texture_id);
        free(rgba_data);
        return 0;
    }
    
    // Store texture info
    g_current_texture.texture_id = texture_id;
    g_current_texture.width = width;
    g_current_texture.height = height;
    strncpy(g_current_texture.name, asset_name, sizeof(g_current_texture.name) - 1);
    g_current_texture.loaded = true;
    
    // Enhanced debug pixel sampling
    if (g_texture_debug && bytes_read >= 16) {
        printf("🎨 Enhanced pixel analysis for %s:\n", asset_name);
        printf("   📊 Data size: %d bytes, Expected: %d bytes\n", (int)bytes_read, expected_size);
        printf("   📐 Dimensions: %dx%d (%s)\n", width, height, format_name);
        
        // DEBUG: Show first 8 pixels as RGBA values
        printf("   🎨 First 8 pixels (RGBA): ");
        for (int i = 0; i < 32 && i < bytes_read; i += 4) {
            printf("(%02x,%02x,%02x,%02x) ", 
                   rgba_data[i], rgba_data[i+1], rgba_data[i+2], rgba_data[i+3]);
            if ((i/4 + 1) % 4 == 0) printf("\n                              ");
        }
        printf("\n");
        
        // Show as hex dump for pattern analysis
        printf("   📋 Raw hex (first 32 bytes): ");
        for (int i = 0; i < 32 && i < bytes_read; i++) {
            printf("%02x ", rgba_data[i]);
            if ((i + 1) % 16 == 0) printf("\n                                ");
        }
        printf("\n");
    }
    
    free(rgba_data);
    printf("✅ Enhanced texture loaded: %dx%d, ID=%u\n", width, height, texture_id);
    return texture_id;
}

// Adaptive texture rendering based on actual dimensions
void enhanced_render_texture(void) {
    if (!g_current_texture.loaded || g_current_texture.texture_id == 0) {
        // Render fallback indicator
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.5f, 0.0f, 0.5f); // Purple for "no texture"
        glBegin(GL_QUADS);
        glVertex3f(-0.2f, -0.2f, 0.0f);
        glVertex3f(0.2f, -0.2f, 0.0f);
        glVertex3f(0.2f, 0.2f, 0.0f);
        glVertex3f(-0.2f, 0.2f, 0.0f);
        glEnd();
        return;
    }
    
    // Calculate adaptive rendering size based on texture dimensions  
    float base_size = 0.3f; // Base size in normalized coordinates
    
    // Scale up small textures so they're visible
    if (g_current_texture.width <= 8 && g_current_texture.height <= 8) {
        base_size = 0.5f; // Very large for tiny textures (8x8 or smaller)
    } else if (g_current_texture.width <= 16 && g_current_texture.height <= 16) {
        base_size = 0.4f; // Larger for small textures (16x16)
    } else if (g_current_texture.width <= 32 && g_current_texture.height <= 32) {
        base_size = 0.35f; // Medium for standard textures (32x32)
    } else if (g_current_texture.width >= 64 || g_current_texture.height >= 64) {
        base_size = 0.25f; // Smaller for large textures (64x64+)
    }
    
    float aspect_ratio = (float)g_current_texture.width / (float)g_current_texture.height;
    
    float render_width, render_height;
    if (aspect_ratio > 1.0f) {
        // Wider than tall
        render_width = base_size;
        render_height = base_size / aspect_ratio;
    } else {
        // Taller than wide or square
        render_width = base_size * aspect_ratio;
        render_height = base_size;
    }
    
    // Center the texture
    float x1 = -render_width * 0.5f;
    float y1 = -render_height * 0.5f;
    float x2 = render_width * 0.5f;
    float y2 = render_height * 0.5f;
    
    printf("🖼️  Rendering %s: %dx%d -> %.2fx%.2f (aspect %.2f)\n", 
           g_current_texture.name, g_current_texture.width, g_current_texture.height,
           render_width, render_height, aspect_ratio);
    
    // Check for OpenGL errors before rendering
    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        printf("⚠️  OpenGL error before enhanced render: 0x%x\n", gl_error);
    }
    
    // Render with proper texture coordinates
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_current_texture.texture_id);
    glColor3f(1.0f, 1.0f, 1.0f); // Full white for accurate colors
    
    glBegin(GL_QUADS);
    // Bottom-left
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, 0.0f);
    // Bottom-right  
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y1, 0.0f);
    // Top-right
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x2, y2, 0.0f);
    // Top-left
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x1, y2, 0.0f);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    
    // Check for OpenGL errors after rendering
    gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        printf("⚠️  OpenGL error after enhanced render: 0x%x\n", gl_error);
    }
}

// Enhanced debug overlay
void enhanced_render_debug_overlay(void) {
    if (!g_current_texture.loaded) return;
    
    // Render texture info text overlay (simple colored rectangles as indicators)
    // Top indicator: texture size category
    glDisable(GL_TEXTURE_2D);
    
    // Color-code by texture size
    if (g_current_texture.width == 16 && g_current_texture.height == 16) {
        glColor3f(0.0f, 1.0f, 0.0f); // Green for 16x16 (AC character)
    } else if (g_current_texture.width == 32) {
        glColor3f(0.0f, 0.0f, 1.0f); // Blue for 32-wide
    } else if (g_current_texture.width == 8) {
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow for 8-wide (small)
    } else {
        glColor3f(1.0f, 0.0f, 1.0f); // Magenta for other sizes
    }
    
    // Small indicator rectangle
    glBegin(GL_QUADS);
    glVertex3f(-0.9f, 0.8f, 0.0f);
    glVertex3f(-0.7f, 0.8f, 0.0f);
    glVertex3f(-0.7f, 0.9f, 0.0f);
    glVertex3f(-0.9f, 0.9f, 0.0f);
    glEnd();
}

// Load test texture function for integration
GLuint enhanced_load_test_texture(const char* asset_name) {
    return enhanced_load_texture(asset_name);
}

// Clean up current texture
void enhanced_cleanup_texture(void) {
    if (g_current_texture.loaded && g_current_texture.texture_id != 0) {
        glDeleteTextures(1, &g_current_texture.texture_id);
        printf("🧹 Cleaned up texture: %s (ID=%u)\n", g_current_texture.name, g_current_texture.texture_id);
    }
    
    memset(&g_current_texture, 0, sizeof(g_current_texture));
}

// Get current texture info
void enhanced_get_texture_info(char* info_buffer, size_t buffer_size) {
    if (g_current_texture.loaded) {
        snprintf(info_buffer, buffer_size, 
                "Texture: %s (%dx%d) ID=%u", 
                g_current_texture.name, 
                g_current_texture.width, 
                g_current_texture.height,
                g_current_texture.texture_id);
    } else {
        snprintf(info_buffer, buffer_size, "No texture loaded");
    }
} 