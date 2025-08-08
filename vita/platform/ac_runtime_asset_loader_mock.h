#pragma once

// Mock header for runtime asset loader
#include "mock_vitagl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Asset loading system for Animal Crossing Vita (Mock Version)

typedef struct {
    char name[64];
    char category[32];
    uint32_t width;
    uint32_t height;
    uint32_t size;
    char path[128];
} ac_runtime_asset_t;

typedef struct {
    GLuint texture_id;
    uint32_t width;
    uint32_t height;
    char name[64];
    bool is_loaded;
} ac_loaded_texture_t;

#define MAX_LOADED_TEXTURES 64
#define AC_ASSETS_BASE_PATH "mock://assets/"

// Runtime asset loader functions
int ac_runtime_init(void);
void ac_runtime_cleanup(void);

// Asset loading
GLuint ac_load_texture_runtime(const char* asset_name);
GLuint ac_load_texture_from_file(const char* asset_name);
bool ac_preload_category(const char* category);
void ac_unload_unused_textures(void);

// Asset management
bool ac_asset_exists(const char* asset_name);
ac_runtime_asset_t* ac_get_asset_info(const char* asset_name);

// Cache management
void ac_clear_texture_cache(void);
int ac_get_cache_usage(void);

// Fallback placeholder textures
GLuint ac_create_placeholder_texture(const char* asset_name);
GLuint ac_create_colored_texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// Utility functions
bool ac_check_assets_available(void);
const char* ac_get_assets_version(void); 