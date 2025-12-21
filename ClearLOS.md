Animal Crossing PC Port - Complete GBI Translation Architecture

Executive Summary

This document provides a comprehensive architectural analysis of both:
1. AC-decomp's current GBI implementation - Our work-in-progress
2. Shipwright's Fast3D implementation - Production reference

The goal is to establish a clear line-of-sight from game code to pixels, identify architectural
mismatches causing rendering failures, and define the correct implementation path.

 ---
Part 1: Side-by-Side Architecture Comparison

AC-decomp Pipeline (Current)

┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 1: GAME CODE                               │
│   Files: src/actor/ac_field_draw.c, src/game/m_play.c              │
│   Generates: Gfx commands to sys_dynamic buffers                    │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 2: GRAPH SYSTEM                            │
│   Files: src/graph.c (graph_draw_finish chains buffers)            │
│   Calls: emu64_taskstart(work_buffer)                              │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│              LAYER 3: GBI INTERPRETER (Our Implementation)          │
│   File: src/pc/gbi_interpreter.c (2011 lines)                      │
│   Problem: Screen-space conversion, hardcoded offsets              │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
OpenGL (Legacy 1.x)

Shipwright Pipeline (Reference)

┌─────────────────────────────────────────────────────────────────────┐
│                    LAYER 1: GAME CODE (OoT/MM)                      │
│   Generates: Standard N64 GBI display lists                         │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│              LAYER 2: FAST3D GBI INTERPRETER                        │
│   File: libultraship/src/graphic/Fast3D/gfx_pc.cpp (2912 lines)    │
│   Key: CPU transforms, clip-space output, state batching           │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
┌─────────────────────────────────────────────────────────────────────┐
│              LAYER 3: RENDERING BACKEND (Abstract API)              │
│   Files: gfx_opengl.cpp, gfx_direct3d*.cpp, gfx_gx2.cpp           │
│   256-triangle batching, shader generation, VBO streaming          │
└───────────────────────────────────┬─────────────────────────────────┘
│
▼
OpenGL 3.3+ / DirectX / GX2

 ---
Part 2: Key Architectural Differences

Difference 1: Coordinate Output

| Aspect     | AC-decomp (Current)              | Shipwright (Correct)           |
 |------------|----------------------------------|--------------------------------|
| VTX output | Screen coords (0-640, 0-480)     | Clip coords (x,y,z,w)          |
| Conversion | Manual NDC conversion with hacks | Let GPU do perspective divide  |
| Y-flip     | Applied in VTX transform         | Applied via invert_y parameter |

AC-decomp Problem (gbi_interpreter.c:790-820):
// WRONG: Converting screen to NDC manually
f32 adjusted_x = raw_x - 960.0f;   // Hardcoded hack!
f32 adjusted_y = raw_y + 200.0f;   // Another hack!
f32 ndc_x = (adjusted_x - vp_trans_x) / vp_scale_x;
f32 ndc_y = -(adjusted_y - vp_trans_y) / vp_scale_y;

Shipwright Solution (gfx_pc.cpp:1011-1100):
// CORRECT: Output clip coords directly
float x = v->ob[0]*MP[0][0] + v->ob[1]*MP[1][0] + v->ob[2]*MP[2][0] + MP[3][0];
// ... same for y, z, w ...
d->x = x;  // Store as clip coords
d->y = y;  // GPU does: NDC = clip / w

Difference 2: Matrix Handling

| Aspect          | AC-decomp            | Shipwright                |
 |-----------------|----------------------|---------------------------|
| Combined matrix | Recalculated per-use | Cached, dirty flag        |
| N64 format      | Custom parser        | Standard parser           |
| Stack depth     | 32 entries           | 11 entries (N64 standard) |

Shipwright's Matrix Multiply (gfx_pc.cpp:919-927):
// Standard row-major 4x4 multiply
for (int i = 0; i < 4; i++) {
for (int j = 0; j < 4; j++) {
res[i][j] = a[i][0]*b[0][j] + a[i][1]*b[1][j] +
a[i][2]*b[2][j] + a[i][3]*b[3][j];
}
}

Difference 3: Segment Address Resolution

| Aspect          | AC-decomp              | Shipwright           |
 |-----------------|------------------------|----------------------|
| Method          | 5-stage fallback chain | Simple segment table |
| 64-bit handling | Base address OR        | Direct storage       |
| Validation      | Weak, allows garbage   | Clean, explicit      |

AC-decomp's Complex Recovery (gbi_interpreter.c:197-293):
// 5 fallback stages - prone to returning garbage
1. Segment table lookup
2. Static pointer registry
3. Terrain pointer special case
4. Static base recovery (g_static_base | addr)
5. Heap base recovery (g_heap_base | addr)

Shipwright's Simple Approach (gfx_pc.cpp:2140-2153):
// Clean segment resolution
uint32_t segNum = (addr >> 24);
uint32_t offset = addr & 0x00FFFFFE;
return (void*)(segmentPointers[segNum] + offset);

Difference 4: Triangle Drawing

| Aspect        | AC-decomp                 | Shipwright                  |
 |---------------|---------------------------|-----------------------------|
| Mode          | Immediate (glBegin/glEnd) | Batched (VBO, 256 tris max) |
| State changes | Per-triangle flush        | Smart dirty tracking        |
| Performance   | Very slow                 | Optimized                   |

AC-decomp Immediate Mode (gbi_interpreter.c:891-920):
glBegin(GL_TRIANGLES);
glVertex4f(v0->x, v0->y, v0->z, v0->w);
glVertex4f(v1->x, v1->y, v1->z, v1->w);
glVertex4f(v2->x, v2->y, v2->z, v2->w);
glEnd();

Shipwright Batched Mode (gfx_pc.cpp:1376-1415):
// Pack vertices into VBO
buf_vbo[buf_vbo_len++] = v->x;
buf_vbo[buf_vbo_len++] = invert_y ? -v->y : v->y;
// ...
if (++buf_vbo_num_tris == MAX_BUFFERED) {
gfx_flush();  // Draw 256 triangles at once
}

Difference 5: Depth Handling

| Aspect     | AC-decomp       | Shipwright             |
 |------------|-----------------|------------------------|
| Z range    | Hardcoded 0.0   | Converted: z = (z+w)/2 |
| Depth test | Disabled (hack) | Proper GL state        |

Shipwright's Depth Conversion (gfx_pc.cpp:1378-1379):
if (clip_parameters.z_is_from_0_to_1) {
z = (z + w) / 2.0f;  // Convert [-w,w] to [0,w]
}

 ---
Part 3: Complete Data Flow Trace

AC-decomp Flow (Current Implementation)

1. GAME CODE (src/actor/*.c, src/game/*.c)
   │
   │  Game calls: gSPVertex(g++, vertices, 8, 0);
   │              gSP1Triangle(g++, 0, 1, 2, 0);
   │
   ▼
2. GFX MACRO (include/pc/gbi.h:355-368)
   │
   │  Encodes: w0 = G_VTX | (n << 12) | (vn << 1)
   │           w1 = (u32)(uintptr_t)vertices  // TRUNCATED to 32-bit!
   │
   ▼
3. DISPLAY LIST BUFFER (sys_dynamic.poly_opa[], etc)
   │
   │  Commands stored as 64-bit Gfx structs: {w0, w1}
   │
   ▼
4. GRAPH SYSTEM (src/graph.c:203-227)
   │
   │  graph_draw_finish() chains buffers:
   │  work -> new0 -> shadow -> new1 -> poly_opa -> poly_xlu -> light -> font -> overlay
   │
   ▼
5. EMU64 COMPAT (src/pc/emu64_compat.c:1-100)
   │
   │  emu64_taskstart(work_buffer) → gbi_execute(work_buffer)
   │
   ▼
6. GBI INTERPRETER (src/pc/gbi_interpreter.c)
   │
   │  gbi_execute_cmd() dispatch loop:
   │
   │  ┌─ G_VTX (opcode 0x04) ──────────────────────────────────────┐
   │  │  1. seg2ptr(w1) - Recover 64-bit pointer (5-stage fallback) │
   │  │  2. memcpy vertices to vertex_buffer[]                      │
   │  │  3. Transform each: pos = model × combined_mtx              │
   │  │  4. Convert to NDC with hardcoded offsets (BROKEN)          │
   │  │  5. Store in transformed[]                                   │
   │  └────────────────────────────────────────────────────────────┘
   │
   │  ┌─ G_TRI1/TRI2 (opcode 0xBF/0xBE) ───────────────────────────┐
   │  │  1. Extract vertex indices (divided by 2 for F3DEX2)        │
   │  │  2. glBegin(GL_TRIANGLES)                                    │
   │  │  3. glVertex4f(transformed[idx].x/y/z/w) for each vertex    │
   │  │  4. glEnd()                                                   │
   │  └────────────────────────────────────────────────────────────┘
   │
   ▼
7. OPENGL
   │
   │  GPU: NDC = clip / w, then viewport transform
   │  BUT: Our coordinates are already mangled by screen-space conversion
   │
   ▼
8. FRAMEBUFFER → SCREEN

Shipwright Flow (Reference)

1. GAME CODE (OoT/MM game code)
   │
   │  Standard N64 GBI calls
   │
   ▼
2. GFX_PC.CPP (gfx_run_dl at line 2162)
   │
   │  Command dispatch switch on opcode
   │
   ▼
3. VERTEX LOAD (gfx_sp_vertex at line 1011)
   │
   │  ┌─────────────────────────────────────────────────────────────┐
   │  │  1. Segment resolution: seg_addr() - simple table lookup     │
   │  │  2. Transform IMMEDIATELY on load:                           │
   │  │     x = ob[0]*MP[0][0] + ob[1]*MP[1][0] + ob[2]*MP[2][0] + MP[3][0]
   │  │  3. Store CLIP COORDS (not screen coords!)                   │
   │  │  4. Calculate trivial rejection flags for clipping           │
   │  └─────────────────────────────────────────────────────────────┘
   │
   ▼
4. TRIANGLE DRAW (gfx_sp_tri1 at line 1206)
   │
   │  ┌─────────────────────────────────────────────────────────────┐
   │  │  1. Check state dirty flags (texture, depth, blend, etc)     │
   │  │  2. Flush if state changed                                   │
   │  │  3. Pack vertex data into VBO buffer:                        │
   │  │     - Position: x, y (flipped if needed), z, w               │
   │  │     - Texture coords, color, fog, etc                        │
   │  │  4. Increment triangle count                                 │
   │  │  5. If 256 triangles buffered: gfx_flush()                   │
   │  └─────────────────────────────────────────────────────────────┘
   │
   ▼
5. GFX_FLUSH (gfx_opengl.cpp:1012-1024)
   │
   │  glBufferData(GL_ARRAY_BUFFER, buf_vbo, GL_STREAM_DRAW)
   │  glDrawArrays(GL_TRIANGLES, 0, 3 * num_tris)
   │
   ▼
6. GPU
   │
   │  Vertex shader: pass-through (positions already transformed)
   │  Fragment shader: Generated based on N64 color combiner mode
   │
   ▼
7. FRAMEBUFFER → SCREEN

 ---
Part 4: Root Cause Analysis

Why Triangles Are Not Visible

Problem 1: Matrices Output Screen Coords (Not Clip Coords)

The N64's RSP does NOT output screen coordinates from the matrix transform. It outputs CLIP
coordinates. The viewport transform (NDC → screen) happens AFTER the triangle is rasterized.

What AC-decomp is doing wrong:
- Assuming combined_mtx outputs screen coords (0-640, 0-480)
- Manually converting back to NDC with hardcoded offsets
- This breaks when terrain matrices have different translations

What Shipwright does correctly:
- combined_mtx (called MP_matrix) outputs CLIP coords
- Just stores x, y, z, w directly
- Lets GPU do perspective divide

Problem 2: Degenerate Triangles from Wrong Vertex Buffer

Debug output shows vertices at same screen position:
idx=1: clip=(-1.9000, 0.1667, 0.0)
idx=2: clip=(-1.9000, 0.1667, 0.0)  <- SAME!

This creates zero-area triangles. The issue is:
1. Vertex data is loaded correctly (terrain corners at 0,10240)
2. But matrix columns for X and Z are collapsed (mp[2][0] = 0)
3. So vz doesn't affect X output at all

Root cause: The terrain matrix is a simple scale+translate:
row0 = (0.06, 0, 0, 0)      <- X = vx * 0.06
row1 = (0, 0.06, 0, 0)      <- Y = vy * 0.06
row2 = (0, 0, 0.06, 0)      <- Z = vz * 0.06 (doesn't affect X!)
row3 = (640, 0, 1920, 1)    <- Translation

This is correct for a 2D orthographic projection of a ground plane viewed from above. But we're then
mangling these coordinates with our screen-to-NDC conversion.

Problem 3: No Proper Camera/View Matrix

Without player/camera initialization, the terrain is being rendered at its world position (centered
around player spawn). The view matrix should translate the world relative to camera, but it's
identity.

 ---
Part 5: Correct Implementation Strategy

Strategy: Align with Shipwright's Approach

We should NOT invent our own coordinate space handling. Instead:

1. Remove all screen-space conversion in gbi_cmd_vtx()
2. Output clip coordinates directly (what Shipwright does)
3. Handle Y-flip at output time, not in transform
4. Handle depth conversion at output time

Implementation Steps

Step 1: Simplify Vertex Transform (gbi_interpreter.c)

Replace the complex screen-to-NDC conversion with direct clip output:

// BEFORE (broken):
f32 adjusted_x = raw_x - 960.0f;
f32 ndc_x = (adjusted_x - vp_trans_x) / vp_scale_x;
dst->x = ndc_x * raw_w;

// AFTER (correct, like Shipwright):
dst->x = raw_x;
dst->y = raw_y;  // Will flip at draw time
dst->z = raw_z;
dst->w = raw_w;

Step 2: Add Y-Flip at Draw Time

In draw_vertex():
// Flip Y for OpenGL coordinate system
glVertex4f(v->x, -v->y, v->z, v->w);

Step 3: Fix Depth Range

In gbi_init() or before drawing:
glDepthRange(0.0, 1.0);  // N64 uses 0-1, not -1 to 1
glDepthFunc(GL_LEQUAL);
glEnable(GL_DEPTH_TEST);

Step 4: Proper Segment Initialization

Ensure all segments are set via G_MOVEWORD/G_MW_SEGMENT before use:
// In game init or first display list:
gMoveWd(gfx++, G_MW_SEGMENT, G_MWO_SEGMENT_0, physical_addr);

Step 5: Fix Camera/View

The terrain renders at world coordinates. We need either:
- Initialize view matrix to center on terrain
- OR stub player position to terrain center

 ---
Part 6: File Reference

AC-decomp Key Files

| File                     | Purpose                 | Key Functions                                |
 |--------------------------|-------------------------|----------------------------------------------|
| src/pc/gbi_interpreter.c | GBI command execution   | gbi_execute(), gbi_cmd_vtx(), gbi_cmd_tri1() |
| src/graph.c              | Display list management | graph_draw_finish(), graph_task_set00()      |
| include/pc/gbi.h         | GBI macros and types    | gSPVertex, gSPMatrix, Gfx, Vtx, Mtx          |
| include/sys_dynamic.h    | Display list buffers    | dynamic_t struct                             |
| src/pc/emu64_compat.c    | EMU64→GBI bridge        | emu64_taskstart()                            |

Shipwright Reference Files

| File                                                | Purpose              | Key Functions
|
|-----------------------------------------------------|----------------------|------------------------
----------------------|
| libultraship/src/graphic/Fast3D/gfx_pc.cpp          | Main GBI interpreter | gfx_run_dl(),
gfx_sp_vertex(), gfx_sp_tri1() |
| libultraship/src/graphic/Fast3D/gfx_pc.h            | RSP state structure  | struct RSP, struct
LoadedVertex              |
| libultraship/src/graphic/Fast3D/gfx_opengl.cpp      | OpenGL backend       |
gfx_opengl_draw_triangles()                  |
| libultraship/src/graphic/Fast3D/gfx_rendering_api.h | Backend abstraction  | struct GfxRenderingAPI
|

 ---
Part 7: Key Insights from Shipwright

What Shipwright Does That We Should Adopt

1. CPU-side vertex transform - Transform vertices immediately on G_VTX load, store clip coords
2. Simple segment table - Just 16 pointers, set via G_MW_SEGMENT
3. Y-flip parameter - Rendering API reports whether to flip Y
4. Depth range conversion - z = (z + w) / 2 for 0-1 range
5. Triangle batching - Buffer up to 256 triangles, single draw call
6. State dirty tracking - Only flush when state actually changes
7. Shader generation - Create shaders on-demand from N64 combiner modes

What Shipwright Does That We Can Skip (For Now)

1. Texture caching - We can use simple immediate binding initially
2. Multiple backends - Start with OpenGL only
3. Shader generation - Use fixed function or simple shaders first
4. Advanced lighting - Stub to flat colors initially

 ---
Part 8: Immediate Action Plan

Priority 1: Fix Coordinate Output

File: src/pc/gbi_interpreter.c
Lines: ~790-830 (gbi_cmd_vtx vertex transform section)

Remove screen-space conversion, output clip coords directly.

Priority 2: Fix Y-Flip

File: src/pc/gbi_interpreter.c
Lines: ~865-880 (draw_vertex function)

Add glVertex4f(v->x, -v->y, v->z, v->w) to flip Y.

Priority 3: Remove Diagnostic Hacks

File: src/pc/gbi_interpreter.c

- Remove forced green color (lines ~844-848)
- Remove hardcoded X/Y offsets (lines ~803-804)
- Re-enable depth testing

Priority 4: Verify with Simple Test

Create a simple test that renders a known triangle at known coordinates to validate the pipeline.

Priority 5: Initialize Camera

Stub player/camera position to center of terrain for initial testing.

 ---
Appendix A: N64 Matrix Format

Memory Layout (64 bytes)

Offset 0x00-0x0F: Integer parts of rows 0-1 (8 x s16)
Offset 0x10-0x1F: Integer parts of rows 2-3 (8 x s16)
Offset 0x20-0x2F: Fractional parts of rows 0-1 (8 x u16)
Offset 0x30-0x3F: Fractional parts of rows 2-3 (8 x u16)

Conversion to Float

// For each element [row][col]:
s16 int_part = read_s16(offset + row*8 + col*2);
u16 frac_part = read_u16(offset + 32 + row*8 + col*2);
float value = (int_part << 16 | frac_part) / 65536.0f;

 ---
Appendix B: GBI Opcode Quick Reference

| F3DEX2 | F3D  | Command    | Description                        |
 |--------|------|------------|------------------------------------|
| 0x01   | 0xDA | G_MTX      | Load/multiply matrix               |
| 0x04   | -    | G_VTX      | Load vertices                      |
| 0x05   | 0xBF | G_TRI1     | Draw 1 triangle                    |
| 0x06   | 0xBE | G_TRI2     | Draw 2 triangles                   |
| 0xDE   | 0x06 | G_DL       | Call/branch display list           |
| 0xDF   | 0xB8 | G_ENDDL    | End display list                   |
| 0xBC   | -    | G_MOVEWORD | Set RSP word (segments, etc)       |
| 0xDC   | 0x03 | G_MOVEMEM  | Load RSP memory (viewport, lights) |