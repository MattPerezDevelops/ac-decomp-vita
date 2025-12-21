/**
 * @file texture_registry.h
 * @brief Texture registry and loading system for PC port
 */

#ifndef PC_TEXTURE_REGISTRY_H
#define PC_TEXTURE_REGISTRY_H

#include "platform.h"

/* Texture format constants */
#define TEX_FMT_CI4     0   /* 4-bit color index (16 colors) */
#define TEX_FMT_CI8     1   /* 8-bit color index (256 colors) */
#define TEX_FMT_RGBA16  2   /* 16-bit RGBA5551 */
#define TEX_FMT_RGBA32  3   /* 32-bit RGBA8888 */
#define TEX_FMT_I4      4   /* 4-bit intensity */
#define TEX_FMT_I8      5   /* 8-bit intensity */
#define TEX_FMT_IA4     6   /* 4-bit intensity + alpha */
#define TEX_FMT_IA8     7   /* 8-bit intensity + alpha */
#define TEX_FMT_IA16    8   /* 16-bit intensity + alpha */

/* Palette registration */
int texture_register_palette(const char* name, u32 dummy_addr,
                             const u16* colors, int color_count);
int texture_find_palette_by_addr(u32 addr);
void texture_set_active_tlut(int palette_idx);

/* Texture registration */
int texture_register(const char* name, u32 dummy_addr,
                     const u8* data, int width, int height, int format);
int texture_find_by_addr(u32 addr);
void texture_set_palette(int tex_idx, int pal_idx);

/* TMEM operations */
void texture_load_to_tmem(int tex_idx);

/* OpenGL integration */
u32 texture_upload_ci4_to_gl(int tex_idx);
int texture_get_current_gl_id(void);
int texture_get_current_width(void);
int texture_get_current_height(void);

/* Initialization */
void texture_init_terrain(void);
void texture_shutdown(void);

#endif /* PC_TEXTURE_REGISTRY_H */
