Animal Crossing PC Port - GBI Translation Architecture

Executive Summary

This document provides a complete architectural analysis of the N64 Graphics Binary Interface (GBI)
translation pipeline for the Animal Crossing PC port. It covers display list flow, address resolution,
coordinate transformations, and identifies issues preventing correct rendering.

 ---
Part 1: Architecture Overview

The Three-Layer Pipeline

┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 1: GAME CODE                               │
│   Actors, field drawing, UI - generates display list commands       │
│   Files: src/actor/ac_field_draw.c, src/game/m_play.c              │
│   Output: Gfx commands written to THA_GA buffers                    │
└─────────────────────────────────────────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 2: GRAPH SYSTEM                            │
│   Buffer management, display list chaining                          │
│   Files: src/graph.c, include/graph.h                              │
│   Output: Chained display lists ready for execution                 │
└─────────────────────────────────────────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 3: GBI INTERPRETER                         │
│   Command dispatch, matrix transforms, OpenGL rendering            │
│   Files: src/pc/gbi_interpreter.c, src/emu64/emu64.c              │
│   Output: OpenGL draw calls                                         │
└─────────────────────────────────────────────────────────────────────┘

 ---
Display List Buffer Structure

The game uses a sys_dynamic structure containing 10 display list buffers:

| Buffer        | Size (Gfx) | Purpose                         | Render Order |
 |---------------|------------|---------------------------------|--------------|
| work          | 128        | Entry point, branches to others | 1st          |
| new0 (bg_opa) | 512        | Background opaque (terrain)     | 2nd          |
| shadow        | 512        | Shadow geometry                 | 3rd          |
| new1 (bg_xlu) | 256        | Background translucent          | 4th          |
| poly_opa      | 9952       | Main opaque polygons            | 5th          |
| poly_xlu      | 2048       | Translucent polygons            | 6th          |
| light         | 256        | Lighting effects                | 7th          |
| font          | 1792       | Text rendering                  | 8th          |
| overlay       | 1024       | UI overlay (ends chain)         | 9th          |

Execution Flow (graph_draw_finish)

work[0] = gSPBranchList(new0)
└→ new0[n] = gSPBranchList(shadow)
└→ shadow[n] = gSPBranchList(new1)
└→ new1[n] = gSPBranchList(poly_opa)
└→ poly_opa[n] = gSPBranchList(poly_xlu)
└→ poly_xlu[n] = gSPBranchList(light)
└→ light[n] = gSPBranchList(font)
└→ font[n] = gSPBranchList(overlay)
└→ overlay[n] = gSPEndDisplayList()

 ---
Static vs Dynamic Display Lists

Static Display Lists

- Definition: Pre-compiled geometry stored in ROM/assets
- Examples: Terrain meshes from segment 10, pre-built culling lists
- Access: Via SEGMENT_ADDR(segment, offset) macro
- PC Challenge: Must be registered via gbi_register_static_ptr()

Dynamic Display Lists

- Definition: Generated at runtime in RAM buffers
- Examples: All THA_GA buffer contents, actor draw commands
- Access: Direct pointer or segment-based
- PC Challenge: Pointer truncation (64-bit → 32-bit → recovery)

 ---
Part 2: Address Resolution System

The Pointer Truncation Problem

On 64-bit systems, display list commands store 32-bit addresses:
gSPDisplayList(g++, terrain_data);  // terrain_data = 0x00007FFF12345678
// Stored in Gfx.w1 as: 0x12345678 (truncated!)

The GBI interpreter must recover the full 64-bit pointer.

seg2ptr() - 5-Stage Recovery (gbi_interpreter.c:199-293)

void* seg2ptr(u32 addr) {
// Stage 1: Segment table lookup
u32 segment = (addr >> 24) & 0x0F;
u32 offset = addr & 0x00FFFFFF;
if (segments[segment] != 0) {
return (void*)(segments[segment] + offset);
}

     // Stage 2: Static pointer registry
     void* static_ptr = lookup_static_ptr(addr);
     if (static_ptr) return static_ptr;

     // Stage 3: Terrain pointer special case
     void* terrain_ptr = test_terrain_get_fullptr(addr);
     if (terrain_ptr) return terrain_ptr;

     // Stage 4: Static base recovery (.data/.bss section)
     u64 recovered = g_static_base | (u32)addr;
     if (is_valid_ptr(recovered)) return (void*)recovered;

     // Stage 5: Heap base recovery (malloc'd memory)
     recovered = g_heap_base | (u32)addr;
     if (recovered > 0x1000) return (void*)recovered;

     return NULL;  // Recovery failed
}

Base Address Concepts

| Base           | Source                           | Purpose                  |
 |----------------|----------------------------------|--------------------------|
| g_static_base  | Upper 32 bits of known .data ptr | Recover static arrays    |
| g_heap_base    | Upper 32 bits of malloc'd ptr    | Recover heap allocations |
| segments[0-15] | Set by G_MW_SEGMENT commands     | N64 segment addressing   |

 ---
Part 3: Matrix Pipeline

Matrix Types and Storage

// gbi_interpreter.c state
f32 modelview_stack[32][4][4];  // Stack with push/pop
int modelview_stack_ptr;         // Current stack top
f32 projection_mtx[4][4];        // Single projection matrix
f32 combined_mtx[4][4];          // MV * P (recomputed on change)

G_MTX Command Processing (gbi_interpreter.c:501-622)

Encoding

w0 = 0xDA000000 | params
params bit 0: PUSH (0=push, 1=no push) - INVERTED by XOR in macro
params bit 1: LOAD (0=multiply, 1=replace)
params bit 2: TYPE (0=modelview, 1=projection)

Matrix Stack Operations

if (params & G_MTX_PROJECTION) {
// Projection: single matrix, no stack
if (params & G_MTX_LOAD) {
projection_mtx = new_mtx;
} else {
projection_mtx = new_mtx * projection_mtx;
}
} else {
// Modelview: stack with push/pop
if (!(params & G_MTX_NOPUSH)) {
stack_ptr++;
stack[stack_ptr] = stack[stack_ptr-1];  // Copy top
}
if (params & G_MTX_LOAD) {
stack[stack_ptr] = new_mtx;
} else {
stack[stack_ptr] = new_mtx * stack[stack_ptr];
}
}
combined_mtx = modelview * projection;  // ALWAYS update

N64 Matrix Format Conversion (mtx_n64_to_float)

N64 stores matrices as interleaved 16.16 fixed-point:
// N64 Mtx structure (64 bytes):
// m[0-1]: High 16 bits of rows 0-3
// m[2-3]: Low 16 bits of rows 0-3

for (int row = 0; row < 4; row++) {
for (int col = 0; col < 4; col++) {
s32 hi = mtx->m[row < 2 ? 0 : 2][row % 2][col];
s32 lo = mtx->m[row < 2 ? 1 : 3][row % 2][col];
s32 fixed = (hi << 16) | (lo & 0xFFFF);
result[row][col] = fixed / 65536.0f;
}
}

 ---
Part 4: Vertex Transformation Pipeline

Coordinate Space Journey

Model Space (Vtx.ob[])     Object/local coordinates
│
▼ combined_mtx (MV * P)
│
Clip Space (x,y,z,w)       Homogeneous coordinates
│
▼ GPU perspective divide (x/w, y/w, z/w)
│
NDC (-1 to +1)             Normalized device coordinates
│
▼ Viewport transform
│
Screen Space (0-640, 0-480) Final pixel coordinates

G_VTX Processing (gbi_interpreter.c:690-838)

Encoding

n  = (w0 >> 12) & 0xFF;   // Vertex count (1-32)
vn = (w0 >> 1) & 0x7F;    // v0 + n
v0 = vn - n;              // Starting buffer index
vtx = seg2ptr(w1);        // Vertex data pointer

Transformation Code

for (int i = 0; i < n; i++) {
// 1. Get model-space position
f32 vx = src->v.ob[0];
f32 vy = src->v.ob[1];
f32 vz = src->v.ob[2];

     // 2. Transform by combined MVP matrix (row-vector convention)
     f32 x = vx*mp[0][0] + vy*mp[1][0] + vz*mp[2][0] + mp[3][0];
     f32 y = vx*mp[0][1] + vy*mp[1][1] + vz*mp[2][1] + mp[3][1];
     f32 z = vx*mp[0][2] + vy*mp[1][2] + vz*mp[2][2] + mp[3][2];
     f32 w = vx*mp[0][3] + vy*mp[1][3] + vz*mp[2][3] + mp[3][3];

     // 3. Store as clip coordinates
     dst->x = x;
     dst->y = y;
     dst->z = z;
     dst->w = w;
}

Important: Row-Vector Convention & Matrix Layout

The N64/F3DEX2 uses row-vector convention (v × M) where the vertex is a row vector multiplied by the
matrix. This is the OPPOSITE of OpenGL's column-vector convention (M × v).

In the GBI, matrices are stored transposed compared to OpenGL's expectation. Our transform code
handles this correctly by accessing mp[col][row] - effectively performing a dot product that respects
the N64 layout:
- mp[0][0], mp[1][0], mp[2][0], mp[3][0] = first row of the logical matrix
- This computes v · row0 for the X component

 ---
Part 5: Screen Space and Viewport

N64 vs OpenGL Coordinate Systems

| Aspect          | N64                        | OpenGL                 |
 |-----------------|----------------------------|------------------------|
| Screen Y origin | TOP (Y=0 at top)           | BOTTOM (Y=0 at bottom) |
| NDC Y range     | -1 (bottom) to +1 (top)    | Same                   |
| Depth range     | 0 to 1.0 (fixed-point)     | -1 to +1 (default NDC) |
| Depth buffer    | Often W-buffer for terrain | Z-buffer               |

W-Buffer vs Z-Buffer

Animal Crossing (like many N64 games) uses W-buffering for terrain to prevent precision artifacts at
distance. If you observe:
- Z values stuck near 0.0
- W values scaling correctly with distance

This is expected behavior! OpenGL will use the W component for:
1. Perspective divide (x/w, y/w, z/w)
2. Perspective-correct texture interpolation

Depth Range Mapping

Critical: N64 depth is 0 to 1.0 (fixed-point), but OpenGL's default NDC depth is -1.0 to +1.0.

Fix options:
// Option 1: Adjust OpenGL depth range to match N64
glDepthRange(0.0, 1.0);

// Option 2: Adjust projection matrix to output (-1, 1) range
// Modify the Z row of projection matrix

Without this fix, you will see Z-fighting artifacts where depth precision is halved.

Viewport Transform

N64 Viewport Structure (Vp_t)

typedef struct {
short vscale[4];  // Scale factors (10.2 fixed-point)
short vtrans[4];  // Translation (10.2 fixed-point)
} Vp_t;

// Standard 640x480 centered viewport:
vscale = {320*4, 240*4, 511*4, 0}  // = {1280, 960, 2044, 0}
vtrans = {320*4, 240*4, 511*4, 0}  // = {1280, 960, 2044, 0}

Viewport Calculation (gbi_interpreter.c:645-682)

f32 scale_x = vscale[0] / 4.0f;  // = 320
f32 scale_y = vscale[1] / 4.0f;  // = 240
f32 trans_x = vtrans[0] / 4.0f;  // = 320
f32 trans_y = vtrans[1] / 4.0f;  // = 240

// N64 viewport: screen = ndc * scale + trans
// OpenGL viewport parameters:
f32 width = 2.0f * scale_x;      // = 640
f32 height = 2.0f * scale_y;     // = 480
f32 x = trans_x - scale_x;       // = 0 (left edge)
f32 y = trans_y - scale_y;       // = 0 (top edge in N64)

// Y-flip for OpenGL (Y=0 at bottom):
int gl_y = 480 - y - height;     // = 0
glViewport(x, gl_y, width, height);

 ---
Part 6: Current Issues and Root Causes

Issue 1: Triangles at Screen Edge (Y=1.0) - The "Smoking Gun"

Symptom: Debug shows pos0=(0.0, 1.0, -0.9) - triangles at top edge

Earlier debug showed values like (1.96, 2.57) for transformed vertices. This is actually correct
behavior:
- In a standard 320×240 N64 projection, a vertex at screen edge has raw_x=320, w=320
- After perspective divide: x/w = 320/320 = 1.0 (exactly at edge)
- Value of 1.96 means the vertex is about one "screen-width" to the right of center
- This confirms the transform is working correctly - the geometry is just positioned off-screen

Root Cause: The N64 combined matrix includes viewport transform, outputting SCREEN coordinates (0-640,
0-480), not clip coordinates. Without camera/player initialization, the terrain is positioned where
the player should be standing (off-screen).

Current Fix (partial):
// Convert screen coords back to NDC
ndc_x = (raw_x - vp_trans_x) / vp_scale_x;
ndc_y = -(raw_y - vp_trans_y) / vp_scale_y;  // Y negated for flip

Problem: This assumes matrices output screen coords, but terrain blocks at different positions have
different built-in translations, causing X offset issues.

Issue 2: X Offset Hack Required

Symptom: Without -320 X offset, terrain renders off-screen right

Root Cause: Camera/player not initialized. Terrain matrices have world-space translations that assume
camera at player position.

Current Hack (gbi_interpreter.c:793):
f32 adjusted_x = raw_x - 320.0f;  // Hardcoded shift

Issue 3: Degenerate Triangles

Symptom: First TRI2 shows v=(0,0,0) - all same vertex

Analysis: This appears to be intentional culling geometry, not a bug. The terrain system uses
degenerate triangles as placeholders.

Issue 4: Garbage Matrix Detection

Symptom: Some matrices rejected as "garbage" (values > 50000)

Root Cause: Address recovery failures cause seg2ptr to return wrong memory, which gets interpreted as
matrix data.

 ---
Part 7: Correct Implementation (Reference: Shipwright)

Key Insights from Shipwright/Fast3D

1. CPU-Based Vertex Transform

Shipwright transforms ALL vertices on CPU before sending to GPU:
// gfx_sp_vertex() - transform immediately on load
x = v->ob[0]*MP[0][0] + v->ob[1]*MP[1][0] + v->ob[2]*MP[2][0] + MP[3][0];
// Store clip coords, NOT screen coords
dst->x = x;  // Let GPU do perspective divide

2. Combined Matrix Always Updated

// After EVERY G_MTX command:
gfx_matrix_mul(rsp.MP_matrix,
rsp.modelview_stack[top],
rsp.P_matrix);

3. Y-Flip at Multiple Levels

// In viewport setup:
area->y = SCREEN_HEIGHT - area->y;

// In vertex output:
buf_vbo[y_index] = invert_y ? -v->y : v->y;

4. Segment Resolution

// Segments are just base addresses
segmentPointers[segNum] = base_address;

// Resolution: base + offset
return segmentPointers[segment] + (addr & 0x00FFFFFF);

 ---
Part 8: Recommended Fixes

Fix 1: Proper Coordinate Output

Current: Matrix outputs screen coords (0-640), converted back to NDC

Correct: Matrix should output clip coords, GPU does perspective divide

// In gbi_cmd_vtx():
// DON'T convert to NDC - output clip coords directly
dst->x = raw_x;
dst->y = raw_y;  // May need Y-flip here or in shader
dst->z = raw_z;
dst->w = raw_w;

// In draw_vertex():
glVertex4f(v->x, v->y, v->z, v->w);  // GPU divides by w

Fix 2: Remove Hardcoded Offsets

Current: X-320 offset, garbage matrix fallbacks

Correct: Initialize camera/view properly at game start

// In m_view.c or game_ct():
// Set camera at map center, looking at terrain
initView_camera_at_terrain_center();

Fix 3: Validate Address Recovery

Current: Multiple fallback stages, some produce garbage

Correct: Strict validation, fail loudly on bad addresses

void* seg2ptr(u32 addr) {
void* result = try_all_recovery_methods(addr);
if (!result || !is_readable(result)) {
printf("[seg2ptr] FAILED: 0x%08X\n", addr);
return NULL;  // Don't return garbage
}
return result;
}

Fix 4: Handle Guardband Clipping

The Problem: The N64 RSP has a "guardband" that allows vertices to exist slightly outside the (-1, 1)
NDC range before clipping them. OpenGL is stricter - vertices outside NDC are immediately clipped.

Symptom: Triangles disappearing at the very edge of the window

Fix Options:
// Option 1: Implement software guardband clipping
// Clip triangles to slightly larger bounds before sending to GPU

// Option 2: Adjust viewport to be slightly larger than window
glViewport(-32, -32, 640+64, 480+64);  // Add guardband margin

// Option 3: Use clip distance in shader (modern OpenGL)
gl_ClipDistance[0] = guardband_check(position);

Fix 5: Depth Range Configuration

// In gbi_init() or before rendering:
glDepthRange(0.0, 1.0);  // Match N64's 0-1 depth range
glDepthFunc(GL_LEQUAL);  // Standard depth test
glEnable(GL_DEPTH_TEST);

Fix 6: Clean Up Diagnostic Hacks

Remove after correct rendering verified:
- Forced green vertex colors (lines 829-832)
- Disabled depth test (lines 898, 939, 952)
- X-320 offset (line 793)
- Garbage matrix fallback identity (lines 552-559)

 ---
Part 9: Implementation Checklist

Immediate Actions

1. Verify Y-flip is applied correctly
- Check that NDC Y is negated
- Verify glViewport Y is flipped
- Test with simple known geometry
2. Fix depth range mapping
- Add glDepthRange(0.0, 1.0) to match N64
- Set glDepthFunc(GL_LEQUAL)
- Re-enable depth testing once working
3. Fix camera initialization
- Ensure initView() sets camera at terrain center
- Remove hardcoded X offset once camera works
4. Validate seg2ptr recovery
- Add verbose logging for failed recoveries
- Track which addresses fail most often
5. Compare matrix values with Shipwright
- Log MP matrix values
- Compare with known-good implementation
6. Consider guardband clipping
- Test if triangles disappear at screen edges
- Implement software clipping if needed

Files to Modify

| File                     | Changes                            |
 |--------------------------|------------------------------------|
| src/pc/gbi_interpreter.c | Fix coordinate output, clean hacks |
| src/game/m_view.c        | Proper camera initialization       |
| src/pc/linker_stubs.c    | Camera/player stubs                |

Success Criteria

1. Terrain triangles visible without hardcoded offsets
2. Triangles fill expected screen area (not just edges)
3. Depth ordering correct (no z-fighting)
4. No garbage matrices in logs
5. All diagnostic hacks removed

 ---
Appendix A: Key Function Reference

| Function               | File                | Lines     | Purpose               |
 |------------------------|---------------------|-----------|-----------------------|
| seg2ptr()              | gbi_interpreter.c   | 199-293   | Address recovery      |
| gbi_cmd_mtx()          | gbi_interpreter.c   | 501-622   | Matrix command        |
| gbi_cmd_vtx()          | gbi_interpreter.c   | 690-838   | Vertex load/transform |
| gbi_cmd_tri1()         | gbi_interpreter.c 1 | 874-911   | Triangle draw         |
| gbi_cmd_tri2()         | gbi_interpreter.c   | 916-957   | Two triangles         |
| gbi_cmd_set_viewport() | gbi_interpreter.c   | 645-682   | Viewport setup        |
| graph_task_set00()     | graph.c             | 154-201   | DL execution          |
| graph_draw_finish()    | graph.c             | 203-334   | Chain DL buffers      |
| emu64_taskstart()      | emu64.c             | 5435-5447 | Original GC entry     |

 ---
Appendix B: GBI Command Quick Reference

| Opcode    | Command       | w0 Format     | w1 Format      |
 |-----------|---------------|---------------|----------------|
| 0x01/0xDA | G_MTX         | params        | matrix address |
| 0x04      | G_VTX         | n, vn         | vertex address |
| 0x05/0xBF | G_TRI1        | -             | v0, v1, v2     |
| 0xBE      | G_TRI2        | v0, v1, v2    | v3, v4, v5     |
| 0x06/0xDE | G_DL          | push/branch   | DL address     |
| 0xDF      | G_ENDDL       | -             | -              |
| 0x03/0xDC | G_MOVEMEM     | index, len    | data address   |
| 0xBC      | G_MOVEWORD    | index, offset | data           |
| 0xE6      | G_RDPLOADSYNC | -             | -              |
| 0xE7      | G_RDPPIPESYNC | -             | -              |