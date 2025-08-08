#pragma once

#include <vitaGL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Enhanced texture renderer with comprehensive GameCube format support
// Based on analysis of 16,361 AC-Decomp files

// Enhanced texture loading with proper GameCube format support
GLuint enhanced_load_texture(const char* asset_name);

// Enhanced texture rendering with adaptive sizing
void enhanced_render_texture(void);

// Get texture info
typedef struct {
    int width;
    int height;
    char format_name[32];
} enhanced_texture_info_t;

bool enhanced_get_texture_info(GLuint texture_id, enhanced_texture_info_t* info);

// Cleanup
void enhanced_texture_cleanup(void); 