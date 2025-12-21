# PC Port Rendering Fixes

This document describes the issues found and fixed to get terrain rendering working in the Animal Crossing PC port.

## Summary

The terrain was not rendering despite all individual components appearing to work. The root cause was **missing texture load commands** in the display list, plus incorrect color handling.

---

## Issue 1: Missing G_LOADTEXBLOCK_DOLPHIN Command

**File**: `src/pc/real_terrain.c`

**Problem**: The terrain display list was calling:
1. `G_SETTIMG` - Sets texture address (just stores it)
2. `G_SETTILE_DOLPHIN` - Sets tile properties

But it was **missing** the actual texture load command:
3. `G_LOADTEXBLOCK_DOLPHIN` (opcode 0xD0) - Actually loads texture data

Without this command, the GBI interpreter never called `texture_find_by_seg9_offset()` to look up and upload the terrain textures.

**Fix**: Added `emit_loadtexblock_dolphin()` helper function and called it in the display list:

```c
/* Helper to emit G_LOADTEXBLOCK_DOLPHIN - loads CI4 texture from segment 9 */
static Gfx* emit_loadtexblock_dolphin(Gfx* dl, int width, int height, u32 seg9_offset) {
    dl->words.w0 = _SHIFTL(GBI_G_LOADTEXBLOCK_DOLPHIN, 24, 8) |
                   _SHIFTL(2, 21, 3) |    /* fmt = 2 (CI) */
                   _SHIFTL(0, 19, 2) |    /* siz = 0 (4-bit) */
                   _SHIFTL(width - 1, 8, 8) |
                   (height - 1);
    dl->words.w1 = 0x09000000 | seg9_offset;  /* Segment 9 address */
    return dl + 1;
}
```

**Texture Segment 9 Offsets** (defined in `texture_registry.c`):
- `0x0000`: grass_tex (32x32 CI4)
- `0x1000`: earth_tex (64x64 CI4)
- `0x2000`: cliff_tex (64x64 CI4)
- `0x3000`: bush_a_tex (64x64 CI4)
- `0x4000`: bush_b_tex (64x32 CI4)

---

## Issue 2: Wrong Terrain System Active

**File**: `src/pc/test_terrain.c`

**Problem**: `USE_TERRAIN_MODE` was set to `2` (binary terrain loader) which tried to load a `.bin` file that didn't exist or wasn't properly formatted.

**Fix**: Changed to mode `1` to use `real_terrain.c` which has runtime-built display lists:

```c
#define USE_TERRAIN_MODE 1  /* Use real_terrain.c */
```

---

## Issue 3: Vertex Colors Used Instead of White for Textured Terrain

**File**: `src/pc/gbi_interpreter.c` (in `draw_vertex()`)

**Problem**: Terrain vertex data stores **packed normals** in the color fields (for lighting), not actual colors. Values like `(0, 120, 0, 178)` or `(226, 98, 61, 96)` are normal vectors, not RGB colors.

When `GL_MODULATE` texture environment is used, texture color is multiplied by vertex color. Using normals as colors caused incorrect tinting (red/green tints).

**Fix**: For textured terrain (detected by large clip coordinates), use white instead of vertex colors:

```c
if (g_gbi_state.texture_on && g_gbi_state.gl_texture_valid) {
    /* Model space: small clip coords (logo) - use vertex colors */
    int is_model = (fabsf(clip_x) < 500.0f && fabsf(clip_y) < 500.0f && fabsf(clip_z) < 500.0f);
    if (is_model) {
        glColor4ub(v->r, v->g, v->b, v->a);
    } else {
        /* Terrain: vertex "colors" are normals, use white */
        glColor4ub(255, 255, 255, 255);
    }
}
```

---

## Issue 4: Segment 10 Address Encoding (Fixed Earlier)

**File**: `src/actor/ac_field_draw.c`

**Problem**: `SEGMENT_ADDR(G_MWO_SEGMENT_A, 0)` produced `0x28000000` instead of `0x0A000000`.

`G_MWO_SEGMENT_A = 0x28` is a **byte offset** for `gSPSegment`, but `SEGMENT_ADDR` expects a **segment number**.

**Fix**: Use literal segment number:
```c
gsSPDisplayList(SEGMENT_ADDR(10, 0)),  /* 0x0A000000 */
```

---

## Known Issue: Nintendo Logo Colors

The Nintendo logo should display with red vertex colors, but currently appears white/gray. The detection heuristic (`clip coords < 500`) may not be correctly identifying logo vertices.

**Potential causes**:
- Logo vertices may have clip_w != 1.0 (perspective projection)
- Logo may be rendered through a different code path (G_TRI2 vs G_TRI1)
- Color state may be getting reset between logo frames

**TODO**: Investigate logo rendering path and fix color detection.

---

## Architecture Notes

### Texture Loading Flow
```
1. G_LOADTEXBLOCK_DOLPHIN command in display list
   ↓
2. gbi_cmd_loadtexblock_dolphin() extracts segment 9 offset
   ↓
3. texture_find_by_seg9_offset() maps offset to texture name
   ↓
4. texture_upload_ci4_to_gl() expands CI4 → RGBA8888 using palette
   ↓
5. glTexImage2D() uploads to OpenGL
   ↓
6. g_gbi_state.gl_texture_id set for subsequent triangles
```

### Terrain Display List Structure
```
G_GEOMETRYMODE (clear lighting)
G_GEOMETRYMODE (set Z-buffer, shade, cull, smooth)
G_RDPPIPESYNC
G_LOADTEXBLOCK_DOLPHIN (grass 32x32)
G_TEXTURE (enable)
G_VTX (load vertices)
G_TRI1 (draw triangles)
... repeat for each texture/batch ...
G_ENDDL
```

### Key Files
- `src/pc/real_terrain.c` - Runtime terrain display list builder
- `src/pc/texture_registry.c` - Texture/palette registry and CI4 expansion
- `src/pc/gbi_interpreter.c` - GBI command interpreter
- `src/pc/test_terrain.c` - Terrain system selector
- `src/pc/assets_terrain.c` - Embedded texture/palette data
