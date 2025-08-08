#!/usr/bin/env python3
"""
Comprehensive GameCube Format Analysis for Animal Crossing
Identifies all texture formats and dimensions we need to support
"""

import os
import math

def analyze_gamecube_formats():
    """Analyze all GameCube texture formats found in AC-Decomp"""
    print("🎮 Comprehensive GameCube Format Analysis")
    print("=" * 60)
    
    # Common GameCube texture sizes found in AC
    observed_sizes = [32, 64, 128, 256, 448, 512, 912, 1008, 1136]
    
    print("📊 Format Analysis by File Size:")
    print("-" * 60)
    
    comprehensive_formats = {}
    
    for size in observed_sizes:
        print(f"\n{size} bytes:")
        formats = []
        
        # CI4 Analysis (4-bit indexed, 0.5 bytes per pixel)
        ci4_pixels = size * 2
        ci4_dims = calculate_possible_dimensions(ci4_pixels)
        if ci4_dims:
            formats.append(f"CI4: {ci4_dims} ({ci4_pixels} pixels)")
        
        # CI8 Analysis (8-bit indexed, 1 byte per pixel)
        ci8_pixels = size
        ci8_dims = calculate_possible_dimensions(ci8_pixels)
        if ci8_dims:
            formats.append(f"CI8: {ci8_dims} ({ci8_pixels} pixels)")
        
        # I4 Analysis (4-bit intensity, 0.5 bytes per pixel)
        i4_pixels = size * 2
        i4_dims = calculate_possible_dimensions(i4_pixels)
        if i4_dims:
            formats.append(f"I4: {i4_dims} ({i4_pixels} pixels)")
        
        # I8 Analysis (8-bit intensity, 1 byte per pixel)
        i8_pixels = size
        i8_dims = calculate_possible_dimensions(i8_pixels)
        if i8_dims:
            formats.append(f"I8: {i8_dims} ({i8_pixels} pixels)")
        
        # RGB5A3 Analysis (16-bit color, 2 bytes per pixel)
        if size % 2 == 0:
            rgb_pixels = size // 2
            rgb_dims = calculate_possible_dimensions(rgb_pixels)
            if rgb_dims:
                formats.append(f"RGB5A3: {rgb_dims} ({rgb_pixels} pixels)")
        
        # RGBA32 Analysis (32-bit color, 4 bytes per pixel)
        if size % 4 == 0:
            rgba_pixels = size // 4
            rgba_dims = calculate_possible_dimensions(rgba_pixels)
            if rgba_dims:
                formats.append(f"RGBA32: {rgba_dims} ({rgba_pixels} pixels)")
        
        comprehensive_formats[size] = formats
        
        # Print most likely format
        if size == 32:
            print("  🎯 MOST LIKELY: Palette file or 8x8 CI4")
        elif size == 64:
            print("  🎯 MOST LIKELY: Small texture 8x16 CI4 or 8x8 CI8")
        elif size == 256:
            print("  🎯 MOST LIKELY: 32x16 CI4 (like our character textures!)")
        elif size == 512:
            print("  🎯 MOST LIKELY: 32x32 CI4 or 32x16 CI8")
        elif size in [448, 912, 1008, 1136]:
            print("  🎯 MOST LIKELY: Complex texture with padding or unusual format")
        
        for fmt in formats[:3]:  # Show top 3 possibilities
            print(f"    ✅ {fmt}")
        if len(formats) > 3:
            print(f"    ... and {len(formats) - 3} more possibilities")
    
    return comprehensive_formats

def calculate_possible_dimensions(pixels):
    """Calculate possible width/height combinations for a given pixel count"""
    possible = []
    
    # Perfect squares
    sqrt_val = int(math.sqrt(pixels))
    if sqrt_val * sqrt_val == pixels:
        possible.append(f"{sqrt_val}x{sqrt_val}")
    
    # Common GameCube dimensions (powers of 2 and multiples)
    common_widths = [4, 8, 16, 32, 64, 128, 24, 12, 6]
    
    for w in common_widths:
        if pixels % w == 0:
            h = pixels // w
            if h <= 128 and h >= 4:  # Reasonable height limits
                if f"{w}x{h}" not in possible and f"{h}x{w}" not in possible:
                    possible.append(f"{w}x{h}")
    
    return ", ".join(possible[:4])  # Return top 4 possibilities

def generate_enhanced_format_map():
    """Generate comprehensive format mapping for our enhanced renderer"""
    print("\n🔧 Enhanced Format Map for Implementation:")
    print("=" * 60)
    
    format_map = {
        # Palette files (usually 32 bytes = 16 colors * 2 bytes)
        32: {"likely": "Palette", "formats": ["16-color RGB5A3 palette", "8x8 CI4", "4x8 CI8"]},
        
        # Small textures
        64: {"likely": "Small Texture", "formats": ["8x16 CI4", "8x8 CI8", "8x8 I8"]},
        
        # Character textures (our main focus)
        256: {"likely": "Character Texture", "formats": ["32x16 CI4", "16x16 CI8", "16x16 I8"]},
        
        # Medium textures  
        512: {"likely": "Medium Texture", "formats": ["32x32 CI4", "32x16 CI8", "16x16 RGB5A3"]},
        
        # Large/complex textures
        1024: {"likely": "Large Texture", "formats": ["32x32 CI8", "32x32 I8", "16x16 RGBA32"]},
        2048: {"likely": "XL Texture", "formats": ["64x32 CI8", "32x32 RGB5A3", "32x16 RGBA32"]},
        
        # Unusual sizes (might be padded or special formats)
        448: {"likely": "Padded/Special", "formats": ["Custom format with padding"]},
        912: {"likely": "Padded/Special", "formats": ["Custom format with padding"]},
        1008: {"likely": "Padded/Special", "formats": ["Custom format with padding"]},
        1136: {"likely": "Padded/Special", "formats": ["Custom format with padding"]},
    }
    
    print("📋 Implementation Priority:")
    print("  🥇 HIGH PRIORITY (common formats):")
    print("     • 256 bytes → 32x16 CI4 (character textures)")
    print("     • 512 bytes → 32x32 CI4 (standard textures)")
    print("     • 32 bytes → Palette files")
    print("     • 64 bytes → 8x16 CI4 (small textures)")
    
    print("  🥈 MEDIUM PRIORITY (less common):")
    print("     • CI8 variants (256, 512, 1024 bytes)")
    print("     • RGB5A3 direct color textures")
    print("     • I4/I8 intensity textures")
    
    print("  🥉 LOW PRIORITY (unusual):")
    print("     • Padded formats (448, 912, 1008, 1136 bytes)")
    print("     • RGBA32 textures (very large)")
    
    return format_map

def recommend_implementation_updates():
    """Recommend specific updates needed for comprehensive support"""
    print("\n🚀 Implementation Recommendations:")
    print("=" * 60)
    
    print("1. 📊 Enhanced Dimension Detection:")
    print("   Update enhanced_texture_renderer.c to handle:")
    print("   • 32 bytes → Palette file (skip, just log)")
    print("   • 64 bytes → 8x16 format") 
    print("   • 256 bytes → 32x16 CI4 ✅ (already done!)")
    print("   • 512 bytes → 32x32 CI4")
    print("   • 1024 bytes → 32x32 CI8 or 32x32 I8")
    print("   • 2048 bytes → 64x32 or 32x32 RGB5A3")
    
    print("\n2. 🎨 Format Support Priority:")
    print("   Phase 1: CI4 textures (most common) ✅")
    print("   Phase 2: CI8 textures (medium priority)")
    print("   Phase 3: I4/I8 intensity textures")
    print("   Phase 4: RGB5A3 direct color")
    
    print("\n3. 🔧 Converter Updates Needed:")
    print("   • Add CI8 support to tile_aware_converter")
    print("   • Add I4/I8 intensity format support")
    print("   • Add RGB5A3 direct color support")
    print("   • Add padding detection for unusual sizes")
    
    print("\n4. 🎮 Testing Strategy:")
    print("   • Test with different sized textures")
    print("   • Verify all formats render correctly")
    print("   • Check performance with larger textures")

if __name__ == "__main__":
    formats = analyze_gamecube_formats()
    format_map = generate_enhanced_format_map()
    recommend_implementation_updates()
    
    print("\n🎯 SUMMARY:")
    print("✅ Current support: 256-byte CI4 textures (32x16)")
    print("📋 Next priority: 512-byte textures (32x32 CI4)")
    print("🔄 Medium term: CI8, I4, I8 format support")
    print("🚀 Long term: RGB5A3 and unusual format support") 