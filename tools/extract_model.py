#!/usr/bin/env python3
"""
Model Binary Extractor for Animal Crossing PC Port

Extracts vertex and display list data from the decomp's .c/.inc files
and outputs binary blobs that can be loaded at runtime with pointer patching.

Output format:
- .vtx: Raw vertex data (16 bytes per vertex, big-endian)
- .gfx: Raw GBI commands (8 bytes per command, big-endian)
- .meta: JSON metadata (vertex count, command count, pointer offsets)

Usage:
    python3 extract_model.py src/data/model/obj_s_dump.c assets/models/dump_s
"""

import re
import json
import struct
import sys
import os

# GBI command opcodes that contain pointers
# These need to be patched at runtime
POINTER_COMMANDS = {
    0x01: 'G_VTX',           # gsSPVertex - w1 contains vertex pointer
    0xDE: 'G_DL',            # gsSPDisplayList - w1 contains DL pointer
    0xFD: 'G_SETTIMG',       # gsDPSetTextureImage - w1 contains texture pointer
    0xDC: 'G_MOVEMEM',       # gsSPMoveMem - w1 contains memory pointer
}

def parse_vertex_inc(filepath):
    """Parse a vertex .inc file and return list of vertex tuples."""
    vertices = []
    with open(filepath, 'r') as f:
        content = f.read()

    # Match vertex entries: {x, y, z, flag, s, t, r, g, b, a}
    # or {x, y, z, flag, s, t, nx, ny, nz, a}
    pattern = r'\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*\}'

    for match in re.finditer(pattern, content):
        x, y, z = int(match.group(1)), int(match.group(2)), int(match.group(3))
        flag = int(match.group(4))
        s, t = int(match.group(5)), int(match.group(6))
        r, g, b, a = int(match.group(7)), int(match.group(8)), int(match.group(9)), int(match.group(10))
        vertices.append((x, y, z, flag, s, t, r, g, b, a))

    return vertices

def vertex_to_binary(vertex):
    """Convert vertex tuple to 16-byte big-endian binary."""
    x, y, z, flag, s, t, r, g, b, a = vertex
    # Pack as big-endian: 3 signed shorts, 1 unsigned short, 2 signed shorts, 4 unsigned bytes
    return struct.pack('>hhhHhh4B', x, y, z, flag, s, t, r, g, b, a)

def parse_gfx_array(source_code, array_name):
    """
    Parse a Gfx[] array from source code.
    Returns list of (w0, w1, pointer_info) tuples.

    pointer_info is None for non-pointer commands, or
    dict with 'type' and 'symbol' for commands that reference pointers.
    """
    commands = []

    # Find the array definition
    pattern = rf'Gfx\s+{re.escape(array_name)}\s*\[\s*\]\s*=\s*\{{(.*?)\}};'
    match = re.search(pattern, source_code, re.DOTALL)
    if not match:
        print(f"Warning: Could not find Gfx array '{array_name}'")
        return commands

    array_content = match.group(1)

    # Parse individual GBI macro calls
    # Common patterns:
    # gsSPVertex(ptr, n, v0)
    # gsSPDisplayList(dl)
    # gsSPEndDisplayList()
    # gsDPSetTextureImage_Dolphin(fmt, siz, w, h, ptr)
    # etc.

    # For now, we'll identify commands that take pointer arguments
    # and mark them for patching

    # Split by gsSP/gsDP prefix
    macro_pattern = r'(gs[SD]P\w+)\s*\(([^)]*)\)'

    for match in re.finditer(macro_pattern, array_content):
        macro_name = match.group(1)
        args = match.group(2)

        pointer_info = None

        # Identify pointer-containing commands
        if macro_name == 'gsSPVertex':
            # gsSPVertex(ptr, n, v0) - first arg is pointer
            ptr_match = re.match(r'\s*(&?\w+(?:\[\d+\])?)', args)
            if ptr_match:
                pointer_info = {'type': 'vertex', 'symbol': ptr_match.group(1).strip()}

        elif macro_name == 'gsSPDisplayList':
            # gsSPDisplayList(dl) - arg is DL pointer
            ptr_match = re.match(r'\s*(&?\w+)', args)
            if ptr_match:
                pointer_info = {'type': 'displaylist', 'symbol': ptr_match.group(1).strip()}

        elif macro_name in ('gsDPSetTextureImage', 'gsDPSetTextureImage_Dolphin'):
            # Last arg is texture pointer
            arg_list = [a.strip() for a in args.split(',')]
            if arg_list:
                pointer_info = {'type': 'texture', 'symbol': arg_list[-1]}

        elif macro_name == 'gsSPEndDisplayList':
            # No pointer, but we include it
            pass

        # Placeholder w0/w1 - actual values will be extracted differently
        # For now, mark with pointer info for the metadata
        commands.append({
            'macro': macro_name,
            'args': args,
            'pointer_info': pointer_info
        })

    return commands

def extract_model_simple(model_c_path, output_prefix):
    """
    Simple extraction: just parse vertex data and output binary.
    For display lists, we'll need a more sophisticated approach.
    """
    base_dir = os.path.dirname(model_c_path)
    model_name = os.path.splitext(os.path.basename(model_c_path))[0]

    # Read the source file
    with open(model_c_path, 'r') as f:
        source = f.read()

    # Find vertex includes
    vtx_pattern = r'Vtx\s+(\w+)\s*\[\s*\]\s*=\s*\{\s*#include\s+"assets/(\w+\.inc)"'
    vtx_matches = re.findall(vtx_pattern, source)

    vertices_data = {}
    for var_name, inc_file in vtx_matches:
        # Find the .inc file
        inc_path = os.path.join(base_dir, '..', '..', '..', 'build', 'GAFE01_00', 'include', 'assets', inc_file)
        if not os.path.exists(inc_path):
            # Try alternate path
            inc_path = os.path.join(base_dir, 'assets', inc_file)

        if os.path.exists(inc_path):
            vertices = parse_vertex_inc(inc_path)
            vertices_data[var_name] = vertices
            print(f"  Found {len(vertices)} vertices in {var_name}")
        else:
            print(f"  Warning: Could not find {inc_file}")

    # Create output directory
    os.makedirs(os.path.dirname(output_prefix) or '.', exist_ok=True)

    # Write vertex binaries
    for var_name, vertices in vertices_data.items():
        vtx_path = f"{output_prefix}_{var_name}.vtx"
        with open(vtx_path, 'wb') as f:
            for v in vertices:
                f.write(vertex_to_binary(v))
        print(f"  Wrote {vtx_path} ({len(vertices) * 16} bytes)")

    # Create metadata
    metadata = {
        'source': model_c_path,
        'vertices': {name: len(verts) for name, verts in vertices_data.items()},
        'endian': 'big',
        'vtx_size': 16,
        'gfx_size': 8,
    }

    meta_path = f"{output_prefix}.meta.json"
    with open(meta_path, 'w') as f:
        json.dump(metadata, f, indent=2)
    print(f"  Wrote {meta_path}")

    return metadata

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 extract_model.py <model.c> <output_prefix>")
        print("Example: python3 extract_model.py src/data/model/obj_s_dump.c assets/models/dump_s")
        sys.exit(1)

    model_path = sys.argv[1]
    output_prefix = sys.argv[2]

    print(f"Extracting model from {model_path}")
    extract_model_simple(model_path, output_prefix)
    print("Done!")

if __name__ == '__main__':
    main()
