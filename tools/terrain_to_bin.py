#!/usr/bin/env python3
"""
terrain_to_bin.py - Universal Terrain Translator

Parses original grd_s_*.c terrain files and converts them to binary blobs
that can be loaded at runtime. Handles:
- Dolphin-specific GBI commands (texture loads, palettes)
- Packed 5-bit triangle indices (gsSPNTriangles_5b)
- Multi-pass texture rendering
- Vertex array references

Binary Format:
    [Header - 64 bytes]
    - Magic: "ACRE" (4 bytes)
    - Version: u32 (4 bytes) - now version 2
    - Flags: u32 (4 bytes)
    - Vertex offset: u32 (4 bytes)
    - Vertex count: u32 (4 bytes)
    - DL offset: u32 (4 bytes)
    - DL command count: u32 (4 bytes)
    - Texture count: u32 (4 bytes)
    - Texture table offset: u32 (4 bytes)
    - Reserved: 28 bytes

    [Vertex Data]
    - Array of Vtx structures (16 bytes each)

    [Texture Reference Table]
    - Array of texture refs: name_hash(u32), segment_offset(u32)

    [Display List]
    - Array of Gfx commands (8 bytes each)
    - Uses segment 8 for vertices, segment 9 for textures

Usage:
    python terrain_to_bin.py src/data/field/bg/acre/grd_s_c1_1/grd_s_c1_1.c -o c1_1.bin
"""

import argparse
import os
import re
import struct
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Tuple, Optional, Dict

# Binary format constants
MAGIC = b'ACRE'
VERSION = 2
HEADER_SIZE = 64

# Segment assignments
SEG_VERTICES = 8    # Segment 8 for vertex data
SEG_TEXTURES = 9    # Segment 9 for texture data

# GBI Opcodes (F3DEX2/Dolphin hybrid)
G_NOOP = 0x00
G_VTX = 0x01
G_TRI1 = 0xBF
G_TRI2 = 0xB1
G_QUAD = 0xB5
G_ENDDL = 0xDF
G_TEXTURE = 0xD7
G_GEOMETRYMODE = 0xD9
G_SETGEOMETRYMODE = 0xD9
G_SETPRIMCOLOR = 0xFA
G_SETENVCOLOR = 0xFB
G_SETCOMBINE = 0xFC
G_SETTIMG = 0xFD
G_LOADBLOCK = 0xF3
G_LOADTLUT = 0xF0
G_SETTILE = 0xF5
G_SETTILESIZE = 0xF2
G_RDPPIPESYNC = 0xE7
G_RDPLOADSYNC = 0xE6
G_SETOTHERMODE_L = 0xE2
G_SETOTHERMODE_H = 0xE3
G_DL = 0xDE

# Dolphin-specific opcodes
G_LOADTLUT_DOLPHIN = 0xDD
G_LOADTEXBLOCK_DOLPHIN = 0xD0
G_SETTILE_DOLPHIN = 0xD2

# Geometry mode flags
G_ZBUFFER = 0x00000001
G_SHADE = 0x00000004
G_CULL_FRONT = 0x00000200
G_CULL_BACK = 0x00000400
G_FOG = 0x00010000
G_LIGHTING = 0x00020000
G_TEXTURE_GEN = 0x00040000
G_SHADING_SMOOTH = 0x00200000

# Image formats
G_IM_FMT_CI = 2
G_IM_FMT_RGBA = 0
G_IM_FMT_I = 4
G_IM_FMT_IA = 3

# GX wrap modes
GX_CLAMP = 0
GX_REPEAT = 1
GX_MIRROR = 2


@dataclass
class Vtx:
    """N64/GC Vertex structure (Vtx_tn format - 16 bytes)"""
    x: int = 0
    y: int = 0
    z: int = 0
    flag: int = 0
    s: int = 0
    t: int = 0
    r: int = 0
    g: int = 0
    b: int = 0
    a: int = 255

    def to_bytes(self) -> bytes:
        return struct.pack('<hhhHhhBBBB',
            self.x, self.y, self.z, self.flag,
            self.s, self.t,
            self.r, self.g, self.b, self.a)


@dataclass
class Gfx:
    """GBI command (8 bytes)"""
    w0: int = 0
    w1: int = 0

    def to_bytes(self) -> bytes:
        return struct.pack('<II', self.w0, self.w1)


@dataclass
class TextureRef:
    """Reference to a texture by name"""
    name: str
    name_hash: int = 0

    def __post_init__(self):
        # Simple hash of texture name
        self.name_hash = hash(self.name) & 0xFFFFFFFF


class TerrainParser:
    """Parses terrain .c files and generates binary display lists"""

    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.vertices: List[Vtx] = []
        self.commands: List[Gfx] = []
        self.texture_refs: List[TextureRef] = []
        self.texture_name_to_idx: Dict[str, int] = {}

        # Known texture names -> segment offsets
        # These will be resolved at runtime by the texture registry
        self.texture_offsets = {
            'grass_tex_dummy': 0x0000,
            'earth_tex_dummy': 0x1000,
            'cliff_tex_dummy': 0x2000,
            'bush_a_tex_dummy': 0x3000,
            'bush_b_tex_dummy': 0x4000,
            'bush_pal_dummy': 0x5000,
            'earth_pal_dummy': 0x5100,
            'cliff_pal_dummy': 0x5200,
        }

    def log(self, msg: str):
        if self.verbose:
            print(f"  {msg}")

    def register_texture(self, name: str) -> int:
        """Register a texture reference and return its index"""
        if name not in self.texture_name_to_idx:
            idx = len(self.texture_refs)
            self.texture_refs.append(TextureRef(name))
            self.texture_name_to_idx[name] = idx
            self.log(f"Registered texture '{name}' as index {idx}")
        return self.texture_name_to_idx[name]

    def get_texture_segaddr(self, name: str) -> int:
        """Get segmented address for a texture"""
        offset = self.texture_offsets.get(name, 0)
        return (SEG_TEXTURES << 24) | offset

    def parse_vertex_inc(self, inc_path: Path) -> List[Vtx]:
        """Parse a vertex .inc file and return list of Vtx objects"""
        vertices = []

        with open(inc_path, 'r') as f:
            content = f.read()

        # Match flat vertex format: {x, y, z, flag, s, t, r, g, b, a}
        vtx_pattern = r'\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}'

        for match in re.finditer(vtx_pattern, content):
            x, y, z, flag, s, t, r, g, b, a = map(int, match.groups())
            vertices.append(Vtx(x, y, z, flag, s, t, r, g, b, a))

        return vertices

    def emit_gfx(self, w0: int, w1: int):
        """Emit a GBI command"""
        self.commands.append(Gfx(w0 & 0xFFFFFFFF, w1 & 0xFFFFFFFF))

    def emit_texture(self, s_scale: int, t_scale: int, level: int, tile: int, on: int):
        """gsSPTexture - Enable/disable texturing"""
        w0 = (G_TEXTURE << 24) | (level << 11) | (tile << 8) | (on << 1)
        w1 = (s_scale << 16) | t_scale
        self.emit_gfx(w0, w1)
        self.log(f"gsSPTexture on={on}")

    def emit_geometry_mode(self, clear: int, set_mode: int):
        """gsSPGeometryMode - Set geometry mode"""
        w0 = (G_GEOMETRYMODE << 24) | ((~clear) & 0x00FFFFFF)
        w1 = set_mode
        self.emit_gfx(w0, w1)
        self.log(f"gsSPGeometryMode clear=0x{clear:X} set=0x{set_mode:X}")

    def emit_load_geometry_mode(self, mode: int):
        """gsSPLoadGeometryMode - Load geometry mode (clear all, then set)"""
        self.emit_geometry_mode(0xFFFFFF, mode)

    def emit_set_prim_color(self, m: int, l: int, r: int, g: int, b: int, a: int):
        """gsDPSetPrimColor"""
        w0 = (G_SETPRIMCOLOR << 24) | (m << 8) | l
        w1 = (r << 24) | (g << 16) | (b << 8) | a
        self.emit_gfx(w0, w1)
        self.log(f"gsDPSetPrimColor ({r},{g},{b},{a})")

    def emit_rdp_pipe_sync(self):
        """gsDPPipeSync"""
        self.emit_gfx(G_RDPPIPESYNC << 24, 0)

    def emit_set_combine_lerp(self, params: List[str]):
        """gsDPSetCombineLERP - simplified, just emit a basic texture*shade mode"""
        # For now, emit a standard TEXEL0 * SHADE combiner
        # Real implementation would parse all 16 params
        w0 = (G_SETCOMBINE << 24) | 0x127E03  # TEXEL0, SHADE cycle 1
        w1 = 0xFF0FFFFF  # Pass through cycle 2
        self.emit_gfx(w0, w1)
        self.log("gsDPSetCombineLERP (TEXEL0*SHADE)")

    def emit_set_render_mode(self, mode1: str, mode2: str):
        """gsDPSetRenderMode - emit basic render mode"""
        # Simplified: just emit a ZB_OPA mode
        # G_SETOTHERMODE_L with render mode bits
        w0 = (G_SETOTHERMODE_L << 24) | (3 << 8) | 29  # shift=3, len=29
        w1 = 0x00552078  # Basic ZB OPA mode
        self.emit_gfx(w0, w1)
        self.log(f"gsDPSetRenderMode {mode1}, {mode2}")

    def emit_load_tlut_dolphin(self, tile: int, count: int, mode: int, pal_name: str):
        """gsDPLoadTLUT_Dolphin - Load palette (Dolphin format)"""
        # Register texture and get segmented address
        self.register_texture(pal_name)
        seg_addr = self.get_texture_segaddr(pal_name)

        # Emit as Dolphin LOADTLUT command
        w0 = (G_LOADTLUT_DOLPHIN << 24) | (tile << 16) | count
        w1 = seg_addr
        self.emit_gfx(w0, w1)
        self.log(f"gsDPLoadTLUT_Dolphin tile={tile} count={count} addr=0x{seg_addr:08X}")

    def emit_load_texture_block_dolphin(self, tex_name: str, fmt: int, width: int,
                                         height: int, pal: int, cms: int, cmt: int,
                                         masks: int, maskt: int):
        """gsDPLoadTextureBlock_4b_Dolphin - Load CI4 texture (Dolphin format)"""
        # Register texture and get segmented address
        self.register_texture(tex_name)
        seg_addr = self.get_texture_segaddr(tex_name)

        # Emit as Dolphin LOADTEXBLOCK command
        # w0: opcode | fmt | siz | width-1
        # w1: segmented address
        siz = 0  # G_IM_SIZ_4b
        w0 = (G_LOADTEXBLOCK_DOLPHIN << 24) | (fmt << 21) | (siz << 19) | ((width - 1) << 8) | (height - 1)
        w1 = seg_addr
        self.emit_gfx(w0, w1)

        # Also emit tile settings
        w0_tile = (G_SETTILE_DOLPHIN << 24) | (pal << 20) | (cms << 8) | (cmt << 4) | (masks << 2) | maskt
        w1_tile = (width << 16) | height
        self.emit_gfx(w0_tile, w1_tile)

        self.log(f"gsDPLoadTextureBlock_Dolphin {tex_name} {width}x{height}")

    def emit_vertex(self, vtx_offset: int, count: int, v0: int):
        """gsSPVertex - Load vertices from segment 8"""
        # Calculate segmented address
        seg_addr = (SEG_VERTICES << 24) | (vtx_offset * 16)  # 16 bytes per vertex

        # F3DEX2 format: w0 = opcode | (count << 12) | ((v0 + count) << 1)
        w0 = (G_VTX << 24) | (count << 12) | ((v0 + count) << 1)
        w1 = seg_addr
        self.emit_gfx(w0, w1)
        self.log(f"gsSPVertex offset={vtx_offset} count={count} v0={v0}")

    def emit_tri1(self, v0: int, v1: int, v2: int):
        """gsSP1Triangle"""
        w0 = G_TRI1 << 24
        w1 = ((v0 * 2) << 16) | ((v1 * 2) << 8) | (v2 * 2)
        self.emit_gfx(w0, w1)

    def emit_tri2(self, v0: int, v1: int, v2: int, v3: int, v4: int, v5: int):
        """gsSP2Triangles"""
        w0 = (G_TRI2 << 24) | ((v0 * 2) << 16) | ((v1 * 2) << 8) | (v2 * 2)
        w1 = ((v3 * 2) << 16) | ((v4 * 2) << 8) | (v5 * 2)
        self.emit_gfx(w0, w1)

    def emit_end_dl(self):
        """gsSPEndDisplayList"""
        self.emit_gfx(G_ENDDL << 24, 0)
        self.log("gsSPEndDisplayList")

    def parse_triangles_5b(self, tri_count: int, indices: List[int]):
        """
        Convert packed 5-bit triangles to standard G_TRI1/G_TRI2 commands.
        indices is a flat list of vertex indices (3 per triangle).
        """
        # Filter out padding triangles (0,0,0)
        triangles = []
        for i in range(0, len(indices), 3):
            if i + 2 < len(indices):
                v0, v1, v2 = indices[i], indices[i+1], indices[i+2]
                if not (v0 == 0 and v1 == 0 and v2 == 0):
                    triangles.append((v0, v1, v2))
                elif len(triangles) < tri_count:
                    # Only include (0,0,0) if we haven't reached tri_count yet
                    # This handles cases where vertex 0 is legitimately used
                    if i < tri_count * 3:
                        triangles.append((v0, v1, v2))

        # Emit triangles, preferring G_TRI2 for pairs
        i = 0
        while i < len(triangles):
            if i + 1 < len(triangles):
                # Emit pair as G_TRI2
                t1, t2 = triangles[i], triangles[i + 1]
                self.emit_tri2(t1[0], t1[1], t1[2], t2[0], t2[1], t2[2])
                i += 2
            else:
                # Odd triangle, use G_TRI1
                t = triangles[i]
                self.emit_tri1(t[0], t[1], t[2])
                i += 1

        self.log(f"Emitted {len(triangles)} triangles from 5b format")

    def parse_c_file(self, c_path: Path) -> bool:
        """Parse a terrain .c file and generate GBI commands"""
        with open(c_path, 'r') as f:
            content = f.read()

        # Find vertex include path
        vtx_match = re.search(r'#include\s+"([^"]+_v\.inc)"', content)
        if vtx_match:
            vtx_inc_rel = vtx_match.group(1)
            self.log(f"Vertex include: {vtx_inc_rel}")
        else:
            print("Error: No vertex include found")
            return False

        # Find vertex file in build directory
        project_root = c_path.parent
        while project_root.name != 'ac-decomp' and project_root.parent != project_root:
            project_root = project_root.parent

        vtx_path = project_root / 'build' / 'GAFE01_00' / 'include' / vtx_inc_rel
        if not vtx_path.exists():
            vtx_path = c_path.parent / vtx_inc_rel

        if vtx_path.exists():
            self.vertices = self.parse_vertex_inc(vtx_path)
            self.log(f"Parsed {len(self.vertices)} vertices from {vtx_path}")
        else:
            print(f"Error: Vertex file not found: {vtx_path}")
            return False

        # Find the display list array
        dl_match = re.search(r'extern\s+Gfx\s+(\w+_model)\[\]\s*=\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}',
                            content, re.DOTALL)
        if not dl_match:
            print("Error: No display list found")
            return False

        dl_name = dl_match.group(1)
        dl_content = dl_match.group(2)
        self.log(f"Found display list: {dl_name}")

        # Parse GBI commands
        self.parse_display_list(dl_content)

        return True

    def parse_display_list(self, content: str):
        """Parse GBI macro calls from display list content"""

        # First, normalize multi-line statements:
        # 1. Remove line comments (// ...)
        # 2. Collapse multi-line content
        lines = []
        for line in content.split('\n'):
            # Remove end-of-line comments
            if '//' in line:
                line = line[:line.index('//')]
            lines.append(line.strip())

        # Join all lines and normalize whitespace
        normalized = ' '.join(lines)
        normalized = re.sub(r'\s+', ' ', normalized)

        # Pattern for various GBI commands - now handles content that was multi-line
        patterns = {
            'texture': r'gsSPTexture\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*\)',
            'render_mode': r'gsDPSetRenderMode\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)',
            'combine': r'gsDPSetCombineLERP\s*\([^)]+\)',
            'load_tlut': r'gsDPLoadTLUT_Dolphin\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*\)',
            'load_tex': r'gsDPLoadTextureBlock_4b_Dolphin\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',
            'prim_color': r'gsDPSetPrimColor\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',
            'geom_mode': r'gsSPLoadGeometryMode\s*\(\s*([^)]+)\s*\)',
            'vertex': r'gsSPVertex\s*\(\s*&\w+\[(\d+)\]\s*,\s*(\d+)\s*,\s*(\d+)\s*\)',
            'tri_init': r'gsSPNTrianglesInit_5b\s*\(\s*(\d+)\s*,\s*([^)]+)\)',
            'tri_cont': r'gsSPNTriangles_5b\s*\(\s*([^)]+)\)',
            'end_dl': r'gsSPEndDisplayList\s*\(\s*\)',
        }

        # Track current triangle batch
        current_tri_count = 0
        current_tri_indices = []

        # Find all command matches with their positions to process in order
        all_matches = []

        for cmd_type, pattern in patterns.items():
            for m in re.finditer(pattern, normalized):
                all_matches.append((m.start(), cmd_type, m))

        # Sort by position to maintain command order
        all_matches.sort(key=lambda x: x[0])

        # Process each command in order
        for pos, cmd_type, match in all_matches:

            if cmd_type == 'texture':
                s, t, level, tile, on = match.groups()
                on_val = 1 if on == 'G_ON' else 0
                self.emit_texture(int(s), int(t), int(level), int(tile), on_val)

            elif cmd_type == 'render_mode':
                mode1, mode2 = match.groups()
                self.emit_set_render_mode(mode1, mode2)

            elif cmd_type == 'combine':
                self.emit_set_combine_lerp([])  # Simplified

            elif cmd_type == 'load_tlut':
                tile, count, mode, pal = match.groups()
                self.emit_load_tlut_dolphin(int(tile), int(count), int(mode), pal)

            elif cmd_type == 'load_tex':
                tex, fmt, w, h, pal, cms, cmt, masks, maskt = match.groups()
                fmt_val = G_IM_FMT_CI if 'CI' in fmt else 0
                cms_val = GX_REPEAT if 'REPEAT' in cms else GX_CLAMP
                cmt_val = GX_REPEAT if 'REPEAT' in cmt else GX_CLAMP
                self.emit_load_texture_block_dolphin(tex, fmt_val, int(w), int(h),
                                                     int(pal), cms_val, cmt_val,
                                                     int(masks), int(maskt))

            elif cmd_type == 'prim_color':
                m_val, l, r, g, b, a = map(int, match.groups())
                self.emit_set_prim_color(m_val, l, r, g, b, a)

            elif cmd_type == 'geom_mode':
                mode_str = match.group(1)
                mode = self.parse_geometry_mode(mode_str)
                self.emit_load_geometry_mode(mode)

            elif cmd_type == 'vertex':
                # Flush any pending triangles first
                if current_tri_indices:
                    self.parse_triangles_5b(current_tri_count, current_tri_indices)
                    current_tri_indices = []
                    current_tri_count = 0

                idx, count, v0 = map(int, match.groups())
                self.emit_vertex(idx, count, v0)

            elif cmd_type == 'tri_init':
                # Flush any pending triangles first
                if current_tri_indices:
                    self.parse_triangles_5b(current_tri_count, current_tri_indices)
                    current_tri_indices = []

                count = int(match.group(1))
                indices_str = match.group(2)
                indices = [int(x.strip()) for x in re.findall(r'\d+', indices_str)]
                current_tri_count = count
                current_tri_indices = indices

            elif cmd_type == 'tri_cont':
                indices_str = match.group(1)
                indices = [int(x.strip()) for x in re.findall(r'\d+', indices_str)]
                current_tri_indices.extend(indices)

            elif cmd_type == 'end_dl':
                # Flush any pending triangles
                if current_tri_indices:
                    self.parse_triangles_5b(current_tri_count, current_tri_indices)
                    current_tri_indices = []
                    current_tri_count = 0

                self.emit_end_dl()

        # Final flush
        if current_tri_indices:
            self.parse_triangles_5b(current_tri_count, current_tri_indices)

    def parse_geometry_mode(self, mode_str: str) -> int:
        """Parse geometry mode flags from string"""
        mode = 0
        flags = {
            'G_ZBUFFER': G_ZBUFFER,
            'G_SHADE': G_SHADE,
            'G_CULL_FRONT': G_CULL_FRONT,
            'G_CULL_BACK': G_CULL_BACK,
            'G_FOG': G_FOG,
            'G_LIGHTING': G_LIGHTING,
            'G_TEXTURE_GEN': G_TEXTURE_GEN,
            'G_SHADING_SMOOTH': G_SHADING_SMOOTH,
        }
        for name, val in flags.items():
            if name in mode_str:
                mode |= val
        return mode

    def create_binary(self) -> bytes:
        """Create the binary blob with header, vertices, textures, and display list"""

        # Calculate offsets
        vtx_offset = HEADER_SIZE
        vtx_size = len(self.vertices) * 16

        tex_table_offset = vtx_offset + vtx_size
        tex_table_size = len(self.texture_refs) * 8  # 4 bytes hash + 4 bytes offset

        dl_offset = tex_table_offset + tex_table_size
        dl_size = len(self.commands) * 8

        # Build header (64 bytes)
        # Format: magic(4) + version(4) + flags(4) + vtx_offset(4) + vtx_count(4) +
        #         dl_offset(4) + dl_count(4) + tex_count(4) + tex_offset(4) + reserved(28)
        header = struct.pack('<4sIIIIIIII28s',
            MAGIC,
            VERSION,
            0,  # flags
            vtx_offset,
            len(self.vertices),
            dl_offset,
            len(self.commands),
            len(self.texture_refs),
            tex_table_offset,
            b'\x00' * 28  # reserved
        )

        # Build vertex data
        vtx_data = b''.join(v.to_bytes() for v in self.vertices)

        # Build texture reference table
        tex_data = b''
        for tex in self.texture_refs:
            offset = self.texture_offsets.get(tex.name, 0)
            tex_data += struct.pack('<II', tex.name_hash, offset)

        # Build display list
        dl_data = b''.join(cmd.to_bytes() for cmd in self.commands)

        return header + vtx_data + tex_data + dl_data


def main():
    parser = argparse.ArgumentParser(description='Universal Terrain Translator')
    parser.add_argument('input', help='Input .c file (e.g., grd_s_c1_1.c)')
    parser.add_argument('-o', '--output', help='Output binary file')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')

    args = parser.parse_args()

    input_path = Path(args.input)
    if not input_path.exists():
        print(f"Error: Input file not found: {input_path}")
        return 1

    # Determine output path
    if args.output:
        output_path = Path(args.output)
    else:
        output_path = input_path.with_suffix('.bin')

    print(f"Processing: {input_path}")

    # Parse terrain file
    terrain = TerrainParser(verbose=args.verbose)
    if not terrain.parse_c_file(input_path):
        return 1

    # Create binary
    blob = terrain.create_binary()

    # Write output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(blob)

    print(f"Created: {output_path} ({len(blob)} bytes)")
    print(f"  Vertices: {len(terrain.vertices)}")
    print(f"  Commands: {len(terrain.commands)}")
    print(f"  Textures: {len(terrain.texture_refs)}")

    # List textures used
    if terrain.texture_refs:
        print("  Texture refs:")
        for tex in terrain.texture_refs:
            print(f"    - {tex.name}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
