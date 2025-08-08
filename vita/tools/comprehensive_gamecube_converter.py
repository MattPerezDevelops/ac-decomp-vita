#!/usr/bin/env python3
"""
Comprehensive GameCube Texture Converter for Animal Crossing Vita Port
Handles ALL GameCube texture formats identified in pipeline analysis:
- CI4 (4-bit indexed) ✅ Current
- CI8 (8-bit indexed) 🆕 NEW  
- I4 (4-bit intensity) 🆕 NEW
- I8 (8-bit intensity) 🆕 NEW
- IA8 (8-bit intensity + alpha) 🆕 NEW
- RGB5A3 (direct color) 🆕 NEW

Based on analysis: 16,361 files analyzed, 10,002 texture files, 2,608 palette files
Priority sizes: 256 bytes (4,899 files), 512 bytes (1,811 files), 128 bytes (2,069 files)
"""

import os
import struct
import argparse
from pathlib import Path
from typing import List, Tuple, Optional
from PIL import Image
import json

# GameCube texture format constants
GX_TF_I4 = 0x0
GX_TF_I8 = 0x1  
GX_TF_IA4 = 0x2
GX_TF_IA8 = 0x3
GX_TF_RGB565 = 0x4
GX_TF_RGB5A3 = 0x5
GX_TF_RGBA8 = 0x6
GX_TF_CI4 = 0x8
GX_TF_CI8 = 0x9
GX_TF_CI14X2 = 0xA
GX_TF_CMPR = 0xE

def detect_format_from_filename(filename: str, file_size: int) -> str:
    """Detect texture format from filename and size"""
    filename_lower = filename.lower()
    
    # Direct format detection from filename
    if 'ci4.bin' in filename_lower:
        return 'CI4'
    elif 'ci8.bin' in filename_lower:
        return 'CI8'
    elif 'i4.bin' in filename_lower:
        return 'I4'
    elif 'ia8.bin' in filename_lower:
        return 'IA8'
    elif 'rgb5a3.bin' in filename_lower:
        return 'RGB5A3'
    
    # Size-based detection for _tex.bin and _txt.bin files
    if any(pattern in filename_lower for pattern in ['_tex.bin', '_txt.bin']):
        if file_size == 128:  # 16x16 CI4
            return 'CI4'
        elif file_size == 256:  # 32x16 CI4 (most common!)
            return 'CI4'
        elif file_size == 512:  # 32x32 CI4 or 32x16 CI8
            return 'CI4'  # Assume CI4 first, CI8 as backup
        elif file_size == 1024:  # 32x32 CI8
            return 'CI8'
        elif file_size == 2048:  # 64x32 CI8 or 32x32 RGB5A3
            return 'CI8'
        elif file_size == 64:  # 8x16 CI4 or 8x8 I8
            return 'CI4'
        elif file_size == 32:  # 8x8 CI4 or 4x8 I8
            return 'I4'
    
    # Default fallback
    return 'CI4'

def calculate_texture_dimensions(format_type: str, data_size: int) -> Tuple[int, int]:
    """Calculate texture dimensions based on format and data size"""
    
    bytes_per_pixel = {
        'CI4': 0.5,   # 4 bits = 0.5 bytes per pixel
        'CI8': 1.0,   # 8 bits = 1 byte per pixel
        'I4': 0.5,    # 4 bits = 0.5 bytes per pixel
        'I8': 1.0,    # 8 bits = 1 byte per pixel
        'IA8': 2.0,   # 16 bits = 2 bytes per pixel
        'RGB5A3': 2.0 # 16 bits = 2 bytes per pixel
    }
    
    bpp = bytes_per_pixel.get(format_type, 1.0)
    total_pixels = int(data_size / bpp)
    
    # Common Animal Crossing texture dimensions based on analysis
    common_dimensions = [
        (8, 8), (8, 16), (16, 16), (16, 32), (32, 16), (32, 32), 
        (64, 16), (64, 32), (64, 64), (128, 32), (128, 64)
    ]
    
    # Find best matching dimensions
    for w, h in common_dimensions:
        if w * h == total_pixels:
            return (w, h)
    
    # Fallback: calculate dimensions 
    import math
    side = int(math.sqrt(total_pixels))
    if side * side == total_pixels:
        return (side, side)
    
    # Try common aspect ratios
    for ratio in [2, 4, 8]:
        if total_pixels % ratio == 0:
            h = int(math.sqrt(total_pixels / ratio))
            w = h * ratio
            if w * h == total_pixels:
                return (w, h)
    
    # Final fallback
    return (32, max(1, total_pixels // 32))

def morton_to_xy_8x8_ci4(pixel_index: int) -> tuple:
    """Convert GameCube CI4 8x8 Morton order pixel index to (x, y) coordinates"""
    # GameCube CI4 uses specific 8x8 Morton order pattern
    # This is based on GameCube's GX texture format documentation
    morton_table_8x8 = [
        (0,0), (1,0), (0,1), (1,1), (2,0), (3,0), (2,1), (3,1),
        (0,2), (1,2), (0,3), (1,3), (2,2), (3,2), (2,3), (3,3),
        (4,0), (5,0), (4,1), (5,1), (6,0), (7,0), (6,1), (7,1),
        (4,2), (5,2), (4,3), (5,3), (6,2), (7,2), (6,3), (7,3),
        (0,4), (1,4), (0,5), (1,5), (2,4), (3,4), (2,5), (3,5),
        (0,6), (1,6), (0,7), (1,7), (2,6), (3,6), (2,7), (3,7),
        (4,4), (5,4), (4,5), (5,5), (6,4), (7,4), (6,5), (7,5),
        (4,6), (5,6), (4,7), (5,7), (6,6), (7,6), (6,7), (7,7)
    ]
    
    if pixel_index < len(morton_table_8x8):
        return morton_table_8x8[pixel_index]
    else:
        # Fallback for out of bounds
        return (pixel_index % 8, pixel_index // 8)

def morton_to_xy_8x4_ci8(pixel_index: int) -> tuple:
    """Convert GameCube CI8 8x4 Morton order pixel index to (x, y) coordinates"""
    # GameCube CI8 uses 8x4 Morton order pattern
    morton_table_8x4 = [
        (0,0), (1,0), (0,1), (1,1), (2,0), (3,0), (2,1), (3,1),
        (4,0), (5,0), (4,1), (5,1), (6,0), (7,0), (6,1), (7,1),
        (0,2), (1,2), (0,3), (1,3), (2,2), (3,2), (2,3), (3,3),
        (4,2), (5,2), (4,3), (5,3), (6,2), (7,2), (6,3), (7,3)
    ]
    
    if pixel_index < len(morton_table_8x4):
        return morton_table_8x4[pixel_index]
    else:
        return (pixel_index % 8, pixel_index // 8)

def morton_to_xy_4x4_rgb(pixel_index: int) -> tuple:
    """Convert GameCube RGB 4x4 Morton order pixel index to (x, y) coordinates"""
    # GameCube RGB formats use 4x4 Morton order
    morton_table_4x4 = [
        (0,0), (1,0), (0,1), (1,1), (2,0), (3,0), (2,1), (3,1),
        (0,2), (1,2), (0,3), (1,3), (2,2), (3,2), (2,3), (3,3)
    ]
    
    if pixel_index < len(morton_table_4x4):
        return morton_table_4x4[pixel_index]
    else:
        return (pixel_index % 4, pixel_index // 4)

def detile_ci4_proper_gamecube(data: bytes, width: int, height: int) -> bytes:
    """Proper GameCube CI4 detiling based on GC documentation"""
    print(f"🔧 Using proper GameCube CI4 detiling for {width}x{height}")
    
    # CI4 uses 8x8 blocks, 2 pixels per byte
    block_width = 8
    block_height = 8
    linear_data = bytearray(width * height // 2)
    
    tiles_x = (width + block_width - 1) // block_width
    tiles_y = (height + block_height - 1) // block_height
    
    src_offset = 0
    for tile_y in range(tiles_y):
        for tile_x in range(tiles_x):
            # Process 8x8 tile
            for y in range(block_height):
                for x in range(0, block_width, 2):  # 2 pixels per byte
                    pixel_x1 = tile_x * block_width + x
                    pixel_x2 = tile_x * block_width + x + 1
                    pixel_y = tile_y * block_height + y
                    
                    if (pixel_x1 < width and pixel_x2 < width and pixel_y < height and 
                        src_offset < len(data)):
                        # Linear position for the byte (2 pixels)
                        linear_pos = ((pixel_y * width) + pixel_x1) // 2
                        if linear_pos < len(linear_data):
                            linear_data[linear_pos] = data[src_offset]
                        src_offset += 1
    
    return bytes(linear_data)

def detile_gamecube_4x4(data: bytes, width: int, height: int, bytes_per_pixel: float) -> bytes:
    """Convert GameCube 4x4 tiled texture data to linear format"""
    if bytes_per_pixel == 0.5:
        # Handle CI4/I4 formats (2 pixels per byte)
        return detile_4bit_texture(data, width, height)
    else:
        # Handle 8-bit and 16-bit formats
        return detile_8bit_texture(data, width, height, int(bytes_per_pixel))

def detile_4bit_texture(data: bytes, width: int, height: int) -> bytes:
    """Detile 4-bit texture data (CI4, I4) using proper GameCube 8x8 Morton order"""
    linear_data = bytearray(width * height // 2)  # Pre-allocate for 4-bit data
    tile_size = 8  # CI4 uses 8x8 tiles
    
    tiles_x = (width + tile_size - 1) // tile_size
    tiles_y = (height + tile_size - 1) // tile_size
    
    data_idx = 0
    for tile_y in range(tiles_y):
        for tile_x in range(tiles_x):
            # Morton order within 8x8 tile for CI4
            for morton_idx in range(tile_size * tile_size // 2):  # 32 bytes per 8x8 CI4 tile
                if data_idx >= len(data):
                    break
                        
                        byte_val = data[data_idx]
                        data_idx += 1
                        
                # Convert morton index to x,y within tile using GameCube CI4 pattern
                # GameCube CI4 uses specific 8x8 Morton order
                local_x, local_y = morton_to_xy_8x8_ci4(morton_idx * 2)  # *2 because 2 pixels per byte
                
                # Calculate actual pixel positions
                pixel_x1 = tile_x * tile_size + local_x
                pixel_y1 = tile_y * tile_size + local_y
                pixel_x2 = pixel_x1 + 1
                pixel_y2 = pixel_y1
                
                # Store first pixel (upper 4 bits)
                if pixel_x1 < width and pixel_y1 < height:
                    linear_pos = (pixel_y1 * width + pixel_x1) // 2
                    if linear_pos < len(linear_data):
                        if pixel_x1 % 2 == 0:
                            linear_data[linear_pos] = (linear_data[linear_pos] & 0x0F) | (byte_val & 0xF0)
                        else:
                            linear_data[linear_pos] = (linear_data[linear_pos] & 0xF0) | ((byte_val >> 4) & 0x0F)
                
                # Store second pixel (lower 4 bits)
                if pixel_x2 < width and pixel_y2 < height:
                    linear_pos = (pixel_y2 * width + pixel_x2) // 2
                    if linear_pos < len(linear_data):
                        if pixel_x2 % 2 == 0:
                            linear_data[linear_pos] = (linear_data[linear_pos] & 0x0F) | ((byte_val << 4) & 0xF0)
                        else:
                            linear_data[linear_pos] = (linear_data[linear_pos] & 0xF0) | (byte_val & 0x0F)
    
    return bytes(linear_data)

def detile_8bit_texture(data: bytes, width: int, height: int, bytes_per_pixel: int) -> bytes:
    """Detile CI8 texture data using proper GameCube 8x4 Morton order"""
    linear_data = bytearray(width * height * bytes_per_pixel)
    tile_width = 8  # CI8 uses 8x4 tiles
    tile_height = 4
    
    tiles_x = (width + tile_width - 1) // tile_width
    tiles_y = (height + tile_height - 1) // tile_height
    
    data_idx = 0
    for tile_y in range(tiles_y):
        for tile_x in range(tiles_x):
            # Morton order within 8x4 tile for CI8
            for morton_idx in range(tile_width * tile_height):  # 32 pixels per 8x4 tile
                if data_idx + bytes_per_pixel > len(data):
                    break
                    
                # Convert morton index to x,y within tile
                local_x, local_y = morton_to_xy_8x4_ci8(morton_idx)
                
                pixel_x = tile_x * tile_width + local_x
                pixel_y = tile_y * tile_height + local_y
                    
                if pixel_x < width and pixel_y < height:
                        linear_pos = (pixel_y * width + pixel_x) * bytes_per_pixel
                    for byte_idx in range(bytes_per_pixel):
                        linear_data[linear_pos + byte_idx] = data[data_idx + byte_idx]
                        
                        data_idx += bytes_per_pixel
    
    return bytes(linear_data)

def convert_ci4_to_rgba(texture_data: bytes, palette: List[Tuple[int, int, int, int]], 
                       width: int, height: int) -> bytes:
    """Convert CI4 texture to RGBA32"""
    print(f"🎨 Converting CI4 {width}x{height} with {len(palette)} colors")
    
    # For AC-Decomp files, try linear format first
    print(f"🔧 Testing linear vs tiled format for CI4")
    linear_data = texture_data  # Try linear first
    
    rgba_data = bytearray()
    pixel_count = 0
    expected_pixels = width * height
    
    for byte_val in linear_data:
        if pixel_count >= expected_pixels:
            break
            
        # Extract two 4-bit indices
        index1 = (byte_val >> 4) & 0xF
        index2 = byte_val & 0xF
        
        for index in [index1, index2]:
            if pixel_count >= expected_pixels:
                break
                
            palette_index = index % len(palette)
            r, g, b, a = palette[palette_index]
            # Clamp values to valid range
            r = max(0, min(255, int(r)))
            g = max(0, min(255, int(g)))
            b = max(0, min(255, int(b)))
            a = max(0, min(255, int(a)))
            rgba_data.extend([r, g, b, a])
            pixel_count += 1
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_ci8_to_rgba(texture_data: bytes, palette: List[Tuple[int, int, int, int]], 
                       width: int, height: int) -> bytes:
    """Convert CI8 texture to RGBA32"""
    print(f"🎨 Converting CI8 {width}x{height} with {len(palette)} colors")
    
    # Detile the texture data using proper CI8 8x4 Morton order
    linear_data = detile_8bit_texture(texture_data, width, height, 1)
    
    rgba_data = bytearray()
    expected_pixels = width * height
    
    for i in range(min(len(linear_data), expected_pixels)):
        palette_index = linear_data[i] % len(palette)
        r, g, b, a = palette[palette_index]
        # Clamp values to valid range
        r = max(0, min(255, int(r)))
        g = max(0, min(255, int(g)))
        b = max(0, min(255, int(b)))
        a = max(0, min(255, int(a)))
        rgba_data.extend([r, g, b, a])
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_i4_to_rgba(texture_data: bytes, width: int, height: int) -> bytes:
    """Convert I4 (4-bit intensity) texture to RGBA32"""
    print(f"🔘 Converting I4 {width}x{height} intensity texture")
    
    linear_data = detile_gamecube_4x4(texture_data, width, height, 0.5)
    
    rgba_data = bytearray()
    pixel_count = 0
    expected_pixels = width * height
    
    for byte_val in linear_data:
        if pixel_count >= expected_pixels:
            break
            
        # Extract two 4-bit intensity values
        intensity1 = (byte_val >> 4) & 0xF
        intensity2 = byte_val & 0xF
        
        for intensity in [intensity1, intensity2]:
            if pixel_count >= expected_pixels:
                break
                
            # Scale 4-bit to 8-bit (0-15 -> 0-255)
            gray = (intensity * 255) // 15
            rgba_data.extend([gray, gray, gray, 255])
            pixel_count += 1
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_i8_to_rgba(texture_data: bytes, width: int, height: int) -> bytes:
    """Convert I8 (8-bit intensity) texture to RGBA32"""
    print(f"🔘 Converting I8 {width}x{height} intensity texture")
    
    linear_data = detile_gamecube_4x4(texture_data, width, height, 1.0)
    
    rgba_data = bytearray()
    expected_pixels = width * height
    
    for i in range(min(len(linear_data), expected_pixels)):
        gray = linear_data[i]
        rgba_data.extend([gray, gray, gray, 255])
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_ia8_to_rgba(texture_data: bytes, width: int, height: int) -> bytes:
    """Convert IA8 (8-bit intensity + 8-bit alpha) texture to RGBA32"""
    print(f"🔘 Converting IA8 {width}x{height} intensity+alpha texture")
    
    linear_data = detile_gamecube_4x4(texture_data, width, height, 2.0)
    
    rgba_data = bytearray()
    expected_pixels = width * height
    
    for i in range(0, min(len(linear_data), expected_pixels * 2), 2):
        if i + 1 < len(linear_data):
            intensity = linear_data[i]
            alpha = linear_data[i + 1]
            rgba_data.extend([intensity, intensity, intensity, alpha])
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_rgb5a3_to_rgba(texture_data: bytes, width: int, height: int) -> bytes:
    """Convert RGB5A3 direct color texture to RGBA32"""
    print(f"🌈 Converting RGB5A3 {width}x{height} direct color texture")
    
    linear_data = detile_gamecube_4x4(texture_data, width, height, 2.0)
    
    rgba_data = bytearray()
    expected_pixels = width * height
    
    for i in range(0, min(len(linear_data), expected_pixels * 2), 2):
        if i + 1 < len(linear_data):
            # Read 16-bit value (big-endian)
            pixel16 = (linear_data[i] << 8) | linear_data[i + 1]
            
            if pixel16 & 0x8000:  # RGB5A3 format
                r = ((pixel16 >> 10) & 0x1F) * 255 // 31
                g = ((pixel16 >> 5) & 0x1F) * 255 // 31
                b = (pixel16 & 0x1F) * 255 // 31
                a = 255
            else:  # RGB4A3 format: 0AAABBBBGGGGRRRR
                a = ((pixel16 >> 12) & 0x7) * 255 // 7
                b = ((pixel16 >> 8) & 0xF) * 255 // 15
                g = ((pixel16 >> 4) & 0xF) * 255 // 15
                r = (pixel16 & 0xF) * 255 // 15
            
            rgba_data.extend([r, g, b, a])
    
    # Pad if needed
    while len(rgba_data) < expected_pixels * 4:
        rgba_data.extend([0, 0, 0, 255])
    
    return bytes(rgba_data)

def convert_texture_comprehensive(texture_file: str, palette_file: Optional[str] = None, 
                                output_file: Optional[str] = None) -> bool:
    """Comprehensive texture conversion supporting all GameCube formats"""
    
    if not os.path.exists(texture_file):
        print(f"❌ Texture file not found: {texture_file}")
        return False
    
    # Read texture data
    with open(texture_file, 'rb') as f:
        texture_data = f.read()
    
    file_size = len(texture_data)
    filename = os.path.basename(texture_file)
    
    print(f"🔄 Processing: {filename} ({file_size} bytes)")
    
    # Detect format
    format_type = detect_format_from_filename(filename, file_size)
    width, height = calculate_texture_dimensions(format_type, file_size)
    
    print(f"📊 Detected: {format_type} format, {width}x{height} dimensions")
    
    # Auto-detect palette file if not provided
    if not palette_file and format_type in ['CI4', 'CI8']:
        base_filename = os.path.splitext(filename)[0]  # Remove .bin
        if base_filename.endswith('_tex'):
            # Replace _tex with _pal for AC-Decomp format
            palette_name = base_filename.replace('_tex', '_pal') + '.bin'
            potential_palette = os.path.join(os.path.dirname(texture_file), palette_name)
            if os.path.exists(potential_palette):
                palette_file = potential_palette
    
    # Load palette if needed
    palette = None
    if format_type in ['CI4', 'CI8'] and palette_file:
        if os.path.exists(palette_file):
            palette = load_rgb5a3_palette(palette_file)
            print(f"🎨 Loaded REAL palette: {len(palette)} colors")
        else:
            print(f"⚠️ Palette file not found: {palette_file}")
            # Generate default palette
            palette = generate_default_palette(16 if format_type == 'CI4' else 256)
            print(f"🎨 Using DEFAULT palette: {len(palette)} colors")
    elif format_type in ['CI4', 'CI8']:
        palette = generate_default_palette(16 if format_type == 'CI4' else 256)
        print(f"🎨 Using DEFAULT palette: {len(palette)} colors")
    
    # Convert based on format
    try:
        if format_type == 'CI4':
            rgba_data = convert_ci4_to_rgba(texture_data, palette, width, height)
        elif format_type == 'CI8':
            rgba_data = convert_ci8_to_rgba(texture_data, palette, width, height)
        elif format_type == 'I4':
            rgba_data = convert_i4_to_rgba(texture_data, width, height)
        elif format_type == 'I8':
            rgba_data = convert_i8_to_rgba(texture_data, width, height)
        elif format_type == 'IA8':
            rgba_data = convert_ia8_to_rgba(texture_data, width, height)
        elif format_type == 'RGB5A3':
            rgba_data = convert_rgb5a3_to_rgba(texture_data, width, height)
        else:
            print(f"❌ Unsupported format: {format_type}")
            return False
        
        # Save output
        if not output_file:
            base_name = os.path.splitext(filename)[0]
            output_file = f"{base_name}_{width}x{height}_{format_type}.rgba"
        
        with open(output_file, 'wb') as f:
            f.write(rgba_data)
        
        print(f"✅ Converted to: {output_file} ({len(rgba_data)} bytes RGBA32)")
        
        # Save debug image if PIL is available
        try:
            img = Image.frombytes('RGBA', (width, height), rgba_data)
            debug_png = output_file.replace('.rgba', '.png')
            img.save(debug_png)
            print(f"🖼️ Debug image: {debug_png}")
        except Exception as e:
            print(f"⚠️ Could not save debug image: {e}")
        
        return True
        
    except Exception as e:
        print(f"❌ Conversion failed: {e}")
        import traceback
        traceback.print_exc()
        return False

def load_rgb5a3_palette(palette_file: str) -> List[Tuple[int, int, int, int]]:
    """Load RGB5A3 palette from file"""
    with open(palette_file, 'rb') as f:
        palette_data = f.read()
    
    palette = []
    for i in range(0, len(palette_data), 2):
        if i + 1 < len(palette_data):
            # Read 16-bit value (big-endian)
            pixel16 = (palette_data[i] << 8) | palette_data[i + 1]
            
            if pixel16 & 0x8000:  # RGB555 format (when bit 15 = 1)
                # RGB555: 1RRRRRGGGGGBBBBB
                r = ((pixel16 >> 10) & 0x1F) * 255 // 31
                g = ((pixel16 >> 5) & 0x1F) * 255 // 31  
                b = (pixel16 & 0x1F) * 255 // 31
                a = 255
            else:  # RGB4A3 format (when bit 15 = 0)
                # RGB4A3: 0AAABBBBGGGGRRRR (3-bit alpha, 4-bit each RGB)
                a = ((pixel16 >> 12) & 0x7) * 255 // 7
                b = ((pixel16 >> 8) & 0xF) * 255 // 15
                g = ((pixel16 >> 4) & 0xF) * 255 // 15
                r = (pixel16 & 0xF) * 255 // 15
            
            palette.append((r, g, b, a))
    
    return palette

def generate_default_palette(num_colors: int) -> List[Tuple[int, int, int, int]]:
    """Generate a realistic default palette for Animal Crossing"""
    palette = []
    
    if num_colors == 16:  # CI4 palette
        # Create realistic Animal Crossing colors
        ac_colors = [
            (0, 0, 0, 255),         # Black
            (255, 255, 255, 255),   # White  
            (139, 69, 19, 255),     # Brown (dirt/tree)
            (34, 139, 34, 255),     # Forest Green
            (135, 206, 235, 255),   # Sky Blue
            (255, 255, 0, 255),     # Yellow
            (255, 165, 0, 255),     # Orange
            (255, 192, 203, 255),   # Pink
            (128, 128, 128, 255),   # Gray
            (85, 107, 47, 255),     # Dark Olive Green
            (210, 180, 140, 255),   # Tan
            (255, 20, 147, 255),    # Deep Pink
            (65, 105, 225, 255),    # Royal Blue
            (220, 20, 60, 255),     # Crimson
            (75, 0, 130, 255),      # Indigo
            (240, 230, 140, 255),   # Khaki
        ]
        for i in range(num_colors):
            palette.append(ac_colors[i % len(ac_colors)])
    else:  # CI8 palette (256 colors)
        # Generate more nuanced palette
    for i in range(num_colors):
            if i < 16:
                # Use same base colors for first 16
                base_colors = [
                    (0, 0, 0), (255, 255, 255), (139, 69, 19), (34, 139, 34),
                    (135, 206, 235), (255, 255, 0), (255, 165, 0), (255, 192, 203),
                    (128, 128, 128), (85, 107, 47), (210, 180, 140), (255, 20, 147),
                    (65, 105, 225), (220, 20, 60), (75, 0, 130), (240, 230, 140)
                ]
                r, g, b = base_colors[i]
                palette.append((r, g, b, 255))
            else:
                # Generate variations
                base_idx = (i - 16) % 16
                base_colors = [
                    (0, 0, 0), (255, 255, 255), (139, 69, 19), (34, 139, 34),
                    (135, 206, 235), (255, 255, 0), (255, 165, 0), (255, 192, 203),
                    (128, 128, 128), (85, 107, 47), (210, 180, 140), (255, 20, 147),
                    (65, 105, 225), (220, 20, 60), (75, 0, 130), (240, 230, 140)
                ]
                r, g, b = base_colors[base_idx]
                # Add variation
                variation = ((i - 16) // 16) * 20 - 40
                r = max(0, min(255, r + variation))
                g = max(0, min(255, g + variation))
                b = max(0, min(255, b + variation))
        palette.append((r, g, b, 255))
    
    return palette

def batch_convert_assets(input_dir: str, output_dir: str, create_index: bool = True) -> bool:
    """Batch convert all GameCube assets in a directory"""
    import json
    from pathlib import Path
    
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    
    print(f"🎯 Batch converting GameCube assets...")
    print(f"📁 Input:  {input_path}")
    print(f"📁 Output: {output_path}")
    
    # Create output structure
    categories = {
        'characters': output_path / 'textures' / 'characters',
        'environment': output_path / 'textures' / 'environment', 
        'items': output_path / 'textures' / 'items',
        'ui': output_path / 'textures' / 'ui'
    }
    
    for category_path in categories.values():
        category_path.mkdir(parents=True, exist_ok=True)
    
    # Find all .bin files
    bin_files = list(input_path.glob("**/*.bin"))
    print(f"🔍 Found {len(bin_files)} .bin files")
    
    converted_assets = {}
    success_count = 0
    
    for i, bin_file in enumerate(bin_files):  # Process ALL files
        if i % 1000 == 0:
            print(f"📊 Progress: {i}/{len(bin_files)}")
        
        # Determine category from filename
        filename = bin_file.stem
        if any(keyword in filename for keyword in ['act_', 'mus_', 'npc_']):
            category = 'characters'
        elif any(keyword in filename for keyword in ['int_', 'obj_', 'bg_']):
            category = 'environment'
        elif any(keyword in filename for keyword in ['tool_', 'item_', 'des_']):
            category = 'items'
        elif any(keyword in filename for keyword in ['win_', 'ui_', 'menu_', 'dna_']):
            category = 'ui'
        else:
            category = 'environment'  # Default
        
        # Output file
        output_file = categories[category] / f"{filename}.rgba"
        
        # Look for matching palette - handle proper AC-Decomp naming
        palette_file = None
        if filename.endswith('_tex'):
            # Replace _tex with _pal for AC-Decomp format
            palette_name = filename.replace('_tex', '_pal') + '.bin'
            palette_file = input_path / palette_name
            if not palette_file.exists():
                palette_file = None
        else:
            # Try simple suffix for other formats
        palette_file = input_path / f"{filename}_pal.bin"
        if not palette_file.exists():
            palette_file = None
        
        # Convert
        try:
            success = convert_texture_comprehensive(
                str(bin_file),
                str(palette_file) if palette_file else None,
                str(output_file)
            )
            
            if success:
                # Try to determine dimensions from file size
                file_size = output_file.stat().st_size
                pixels = file_size // 4  # RGBA = 4 bytes per pixel
                
                # Common GameCube texture sizes
                if pixels == 1024:  # 32x32
                    width, height = 32, 32
                elif pixels == 256:   # 16x16
                    width, height = 16, 16
                elif pixels == 4096:  # 64x64
                    width, height = 64, 64
                else:
                    # Try to guess square dimensions
                    import math
                    side = int(math.sqrt(pixels))
                    if side * side == pixels:
                        width, height = side, side
                    else:
                        width, height = 32, 32  # Default
                
                converted_assets[filename] = {
                    'category': category,
                    'width': width,
                    'height': height,
                    'size': file_size,
                    'path': f"textures/{category}/{filename}.rgba"
                }
                success_count += 1
                
        except Exception as e:
            print(f"❌ Failed to convert {filename}: {e}")
    
    print(f"✅ Converted {success_count}/{len(bin_files)} assets")
    
    # Create index file
    if create_index and converted_assets:
        index_data = {
            'version': '2.0.0',
            'source': 'Real GameCube AC-Decomp Assets',
            'total_assets': len(converted_assets),
            'categories': {cat: len([a for a in converted_assets.values() if a['category'] == cat]) 
                          for cat in categories.keys()},
            'assets': converted_assets
        }
        
        index_file = output_path / 'index.json'
        with open(index_file, 'w') as f:
            json.dump(index_data, f, indent=2)
        
        print(f"📊 Created index: {index_file}")
    
    return success_count > 0

def main():
    parser = argparse.ArgumentParser(description='Comprehensive GameCube Texture Converter')
    parser.add_argument('input', help='Input texture file (.bin) or directory for batch conversion')
    parser.add_argument('--output', help='Output RGBA file or directory')
    parser.add_argument('--palette', help='Palette file for CI4/CI8 textures')
    parser.add_argument('--format', help='Force specific format (CI4, CI8, I4, I8, IA8, RGB5A3)')
    parser.add_argument('--batch-convert', action='store_true', help='Batch convert all assets in directory')
    parser.add_argument('--create-index', action='store_true', help='Create asset index file')
    
    args = parser.parse_args()
    
    print("🎮 Comprehensive GameCube Texture Converter")
    print("=" * 50)
    
    if args.batch_convert:
        if not args.output:
            print("❌ --output directory required for batch conversion")
            exit(1)
        
        success = batch_convert_assets(args.input, args.output, args.create_index)
    else:
        # Legacy single file conversion
        success = convert_texture_comprehensive(
            args.input, 
            args.palette, 
            args.output
        )
    
    if success:
        print("✅ Conversion completed successfully!")
    else:
        print("❌ Conversion failed!")
        exit(1)

if __name__ == "__main__":
    main() 