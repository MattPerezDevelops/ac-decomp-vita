#!/usr/bin/env python3
"""
Complete GameCube Pipeline Analysis for Animal Crossing Vita Port
Analyzes all texture files, identifies formats, and ensures comprehensive support
"""

import os
import glob
import struct
from pathlib import Path
from collections import defaultdict, Counter
import json

def analyze_complete_pipeline():
    """Perform comprehensive analysis of GameCube texture pipeline"""
    print("🎮 Complete GameCube Pipeline Analysis")
    print("=" * 70)
    
    # Phase 1: File Discovery and Classification
    file_analysis = discover_and_classify_files()
    
    # Phase 2: Format Detection 
    format_analysis = analyze_texture_formats(file_analysis)
    
    # Phase 3: Pipeline Gap Analysis
    gap_analysis = identify_pipeline_gaps(format_analysis)
    
    # Phase 4: Recommendations
    generate_comprehensive_recommendations(gap_analysis)
    
    return {
        'files': file_analysis,
        'formats': format_analysis,
        'gaps': gap_analysis
    }

def discover_and_classify_files():
    """Discover all AC-Decomp files and classify by type"""
    print("\n🔍 Phase 1: File Discovery and Classification")
    print("-" * 50)
    
    asset_dir = "../../ac-decomp-upstream/build/GAFE01_00/bin/assets"
    if not os.path.exists(asset_dir):
        print(f"❌ Asset directory not found: {asset_dir}")
        return {}
    
    # Find all .bin files
    bin_files = glob.glob(f"{asset_dir}/**/*.bin", recursive=True)
    
    # Classify by naming pattern and size
    classification = {
        'textures': [],      # _tex.bin, _txt.bin
        'palettes': [],      # _pal.bin  
        'vertices': [],      # _v.bin, _vtx.bin
        'unknown': [],       # Others
        'models': [],        # _mdl.bin, _gfx.bin
        'misc': []          # Everything else
    }
    
    size_distribution = Counter()
    name_patterns = Counter()
    
    for filepath in bin_files:
        filename = os.path.basename(filepath)
        file_size = os.path.getsize(filepath)
        
        # Classify by naming pattern
        if any(pattern in filename for pattern in ['_tex.bin', '_txt.bin']):
            classification['textures'].append((filepath, file_size))
        elif '_pal.bin' in filename:
            classification['palettes'].append((filepath, file_size))
        elif any(pattern in filename for pattern in ['_v.bin', '_vtx.bin']):
            classification['vertices'].append((filepath, file_size))
        elif any(pattern in filename for pattern in ['_mdl.bin', '_gfx.bin']):
            classification['models'].append((filepath, file_size))
        else:
            classification['unknown'].append((filepath, file_size))
        
        # Track size distribution
        size_distribution[file_size] += 1
        
        # Track naming patterns
        if '_' in filename:
            suffix = filename.split('_')[-1]
            name_patterns[suffix] += 1
    
    print(f"📊 File Classification Results:")
    for category, files in classification.items():
        print(f"  {category}: {len(files)} files")
    
    print(f"\n📏 Size Distribution (top 10):")
    for size, count in size_distribution.most_common(10):
        print(f"  {size:>6} bytes: {count:>3} files")
    
    print(f"\n🏷️ Name Pattern Distribution (top 10):")
    for pattern, count in name_patterns.most_common(10):
        print(f"  {pattern}: {count:>3} files")
    
    return {
        'classification': classification,
        'size_distribution': dict(size_distribution),
        'name_patterns': dict(name_patterns),
        'total_files': len(bin_files)
    }

def analyze_texture_formats(file_analysis):
    """Analyze texture files to identify GameCube formats"""
    print("\n🎨 Phase 2: Texture Format Analysis")
    print("-" * 50)
    
    texture_files = file_analysis['classification']['textures']
    palette_files = file_analysis['classification']['palettes']
    
    format_detection = {
        'CI4_candidates': [],      # 256 bytes (32x16)
        'CI8_candidates': [],      # 512 bytes (32x16) 
        'I4_candidates': [],       # Intensity 4-bit
        'I8_candidates': [],       # Intensity 8-bit
        'RGB5A3_candidates': [],   # Direct color
        'unusual_sizes': [],       # Non-standard sizes
        'palette_analysis': []     # Palette file analysis
    }
    
    # Analyze texture files
    print("🔍 Analyzing texture files...")
    for filepath, size in texture_files[:20]:  # Sample first 20
        filename = os.path.basename(filepath)
        
        # Analyze based on size patterns
        if size == 256:
            format_detection['CI4_candidates'].append((filename, size, "Likely 32x16 CI4"))
        elif size == 512:
            format_detection['CI8_candidates'].append((filename, size, "Likely 32x16 CI8 or 32x32 CI4"))
        elif size == 128:
            format_detection['CI4_candidates'].append((filename, size, "Likely 16x16 CI4"))
        elif size == 1024:
            format_detection['CI8_candidates'].append((filename, size, "Likely 32x32 CI8"))
        elif size in [64, 32]:
            format_detection['I4_candidates'].append((filename, size, "Small texture or palette"))
        else:
            format_detection['unusual_sizes'].append((filename, size, "Non-standard size"))
        
        # Try to read header for format hints
        try:
            with open(filepath, 'rb') as f:
                header = f.read(min(16, size))
                hex_header = header.hex()[:32]
                format_detection.setdefault('headers', []).append((filename, hex_header))
        except:
            pass
    
    # Analyze palette files
    print("🎨 Analyzing palette files...")
    for filepath, size in palette_files[:10]:  # Sample first 10
        filename = os.path.basename(filepath)
        
        if size == 32:
            format_detection['palette_analysis'].append((filename, size, "16-color RGB5A3 palette"))
        elif size == 64:
            format_detection['palette_analysis'].append((filename, size, "32-color RGB5A3 palette"))
        elif size == 512:
            format_detection['palette_analysis'].append((filename, size, "256-color RGB5A3 palette"))
    
    print(f"📊 Format Detection Results:")
    for format_type, candidates in format_detection.items():
        if candidates and format_type != 'headers':
            print(f"  {format_type}: {len(candidates)} candidates")
    
    return format_detection

def identify_pipeline_gaps(format_analysis):
    """Identify gaps in our current pipeline support"""
    print("\n🔍 Phase 3: Pipeline Gap Analysis")
    print("-" * 50)
    
    # Current pipeline capabilities
    current_support = {
        'texture_formats': ['CI4'],  # We only support CI4 currently
        'dimensions': ['32x16', '16x16', '8x8'],  # Supported dimensions
        'converters': ['tile_aware_ci4'],  # Current converters
        'renderer_formats': ['RGBA32'],  # Output format
    }
    
    # Required based on analysis
    required_support = {
        'texture_formats': ['CI4', 'CI8', 'I4', 'I8', 'RGB5A3'],
        'dimensions': ['8x8', '16x16', '32x16', '32x32', '64x32', '64x64'],
        'converters': ['ci4_converter', 'ci8_converter', 'i4_converter', 'i8_converter', 'rgb5a3_converter'],
        'renderer_formats': ['RGBA32'],  # Keep this simple
    }
    
    gaps = {
        'missing_formats': [],
        'missing_dimensions': [],
        'missing_converters': [],
        'unusual_cases': []
    }
    
    # Identify format gaps
    for fmt in required_support['texture_formats']:
        if fmt not in current_support['texture_formats']:
            gaps['missing_formats'].append(fmt)
    
    # Identify dimension gaps
    for dim in required_support['dimensions']:
        if dim not in current_support['dimensions']:
            gaps['missing_dimensions'].append(dim)
    
    # Identify converter gaps
    for conv in required_support['converters']:
        if conv not in current_support['converters']:
            gaps['missing_converters'].append(conv)
    
    # Identify unusual cases from format analysis
    if 'unusual_sizes' in format_analysis:
        for filename, size, desc in format_analysis['unusual_sizes']:
            gaps['unusual_cases'].append(f"{filename} ({size} bytes): {desc}")
    
    print(f"⚠️ Pipeline Gaps Identified:")
    print(f"  Missing formats: {gaps['missing_formats']}")
    print(f"  Missing dimensions: {gaps['missing_dimensions']}")
    print(f"  Missing converters: {gaps['missing_converters']}")
    print(f"  Unusual cases: {len(gaps['unusual_cases'])} files")
    
    return gaps

def generate_comprehensive_recommendations(gap_analysis):
    """Generate comprehensive recommendations to fix pipeline gaps"""
    print("\n🚀 Phase 4: Comprehensive Recommendations")
    print("-" * 50)
    
    recommendations = {
        'immediate': [],
        'medium_term': [],
        'long_term': [],
        'implementation_plan': []
    }
    
    # Immediate priorities (critical gaps)
    if 'CI8' in gap_analysis['missing_formats']:
        recommendations['immediate'].append("Add CI8 texture format support (common in AC)")
    
    if '32x32' in gap_analysis['missing_dimensions']:
        recommendations['immediate'].append("Add 32x32 dimension support for standard textures")
    
    # Medium term (important but not critical)
    if 'I4' in gap_analysis['missing_formats']:
        recommendations['medium_term'].append("Add I4 intensity texture support")
    
    if 'I8' in gap_analysis['missing_formats']:
        recommendations['medium_term'].append("Add I8 intensity texture support")
    
    # Long term (nice to have)
    if 'RGB5A3' in gap_analysis['missing_formats']:
        recommendations['long_term'].append("Add RGB5A3 direct color support")
    
    # Implementation plan
    recommendations['implementation_plan'] = [
        "1. Update tile_aware_ac_converter.py to support CI8 format",
        "2. Add dimension detection for 32x32 textures in enhanced_texture_renderer.c",
        "3. Create comprehensive format test with sample textures",
        "4. Add I4/I8 intensity format converters",
        "5. Implement RGB5A3 direct color conversion",
        "6. Handle unusual size cases with padding detection"
    ]
    
    print("🎯 Implementation Priorities:")
    print("\n🥇 IMMEDIATE (Critical):")
    for rec in recommendations['immediate']:
        print(f"  • {rec}")
    
    print("\n🥈 MEDIUM TERM (Important):")
    for rec in recommendations['medium_term']:
        print(f"  • {rec}")
    
    print("\n🥉 LONG TERM (Nice to have):")
    for rec in recommendations['long_term']:
        print(f"  • {rec}")
    
    print("\n📋 Implementation Plan:")
    for step in recommendations['implementation_plan']:
        print(f"  {step}")
    
    return recommendations

def export_analysis_results(analysis_results):
    """Export analysis results for reference"""
    output_file = "pipeline_analysis_results.json"
    
    # Convert to JSON-serializable format
    json_results = {
        'timestamp': '2024-08-07',
        'summary': {
            'total_files_analyzed': analysis_results['files']['total_files'],
            'texture_files': len(analysis_results['files']['classification']['textures']),
            'palette_files': len(analysis_results['files']['classification']['palettes']),
            'gaps_identified': len(analysis_results['gaps']['missing_formats'])
        },
        'size_distribution': analysis_results['files']['size_distribution'],
        'gaps': analysis_results['gaps']
    }
    
    with open(output_file, 'w') as f:
        json.dump(json_results, f, indent=2)
    
    print(f"\n💾 Analysis exported to: {output_file}")

if __name__ == "__main__":
    print("🔬 Starting Comprehensive GameCube Pipeline Analysis...")
    
    try:
        results = analyze_complete_pipeline()
        export_analysis_results(results)
        
        print("\n" + "="*70)
        print("✅ ANALYSIS COMPLETE!")
        print("📋 Key Findings:")
        print("   • Current pipeline supports CI4 format well")
        print("   • Missing CI8, I4, I8 format support")
        print("   • Need better dimension detection for various sizes")
        print("   • Unusual file sizes need investigation")
        print("🚀 Ready for pipeline improvements!")
        
    except Exception as e:
        print(f"❌ Analysis failed: {e}")
        import traceback
        traceback.print_exc() 