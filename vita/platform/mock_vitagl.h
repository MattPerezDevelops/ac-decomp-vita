#pragma once

// Mock VitaGL for testing asset bridge without Vita dependencies

#include <stdint.h>

typedef uint32_t GLuint;

// Mock VitaGL functions that we need for testing
inline GLuint glGenTextures(void) { return 1; }
inline void glBindTexture(uint32_t target, GLuint texture) { }
inline void glTexImage2D(uint32_t target, int level, int internalformat, 
                        int width, int height, int border, uint32_t format, 
                        uint32_t type, const void* pixels) { } 