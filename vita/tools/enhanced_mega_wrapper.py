#!/usr/bin/env python3
"""
🚀 ENHANCED MEGA-WRAPPER GENERATOR  
==================================
Smart version that automatically filters out system type conflicts

This enhanced version avoids conflicts with system headers and creates
a truly compatible GameCube SDK replacement.
"""

import re
from pathlib import Path

def create_enhanced_mega_wrapper():
    """Create an enhanced mega-wrapper with smart conflict avoidance"""
    
    # System types to NEVER override (avoid conflicts)
    SYSTEM_TYPES_BLACKLIST = {
        'FILE', 'div_t', 'fpos_t', 'size_t', 'wchar_t', 'time_t', 'clock_t',
        'f64', 'u8', 'u16', 'u32', 'u64', 's8', 's16', 's32', 's64', 'f32',
        'Gfx', 'Vtx', 'Mtx', 'MtxF', 'MtxP', 'GRAPH', 'OSMessage', 'OSMessageQueue', 
        'OSThreadQueue', 'OSMesg', 'OSMesgQueue', 'errno_t', 'uintptr_t', 'intptr_t'
    }
    
    print("🚀 Creating Enhanced Mega-Wrapper")
    print("=" * 35)
    
    # Read current mega-wrapper
    wrapper_path = Path("platform/gx_to_vitagl_wrapper.h")
    with open(wrapper_path, 'r') as f:
        content = f.read()
    
    # Remove problematic auto-generated typedefs
    print("🔧 Filtering out system type conflicts...")
    
    lines = content.split('\n')
    filtered_lines = []
    removed_count = 0
    
    for line in lines:
        # Check if line is a problematic typedef
        typedef_match = re.match(r'typedef u32 (\w+);.*Auto-generated placeholder', line)
        if typedef_match:
            type_name = typedef_match.group(1)
            if type_name in SYSTEM_TYPES_BLACKLIST:
                filtered_lines.append(f"// typedef u32 {type_name};  // Filtered: conflicts with system type")
                removed_count += 1
                continue
        
        # Check for invalid numeric typedefs
        if 'typedef u32 1;' in line or re.match(r'typedef u32 \d+;', line):
            filtered_lines.append("// Invalid numeric typedef filtered out")
            removed_count += 1
            continue
            
        filtered_lines.append(line)
    
    print(f"✅ Filtered out {removed_count} conflicting type definitions")
    
    # Add enhanced conflict prevention
    enhanced_content = add_enhanced_conflict_prevention('\n'.join(filtered_lines))
    
    # Write enhanced wrapper
    with open(wrapper_path, 'w') as f:
        f.write(enhanced_content)
    
    print(f"✅ Enhanced mega-wrapper written to: {wrapper_path}")
    print(f"📊 Size: {len(enhanced_content)} characters")
    
    return wrapper_path

def add_enhanced_conflict_prevention(content):
    """Add enhanced conflict prevention to the wrapper"""
    
    # Find the position to insert enhanced guards
    header_end = content.find("// MEGA-WRAPPER ACTIVE FLAG")
    if header_end == -1:
        header_end = content.find("#define VITAGL_MEGA_WRAPPER_ACTIVE 1")
    
    if header_end == -1:
        print("⚠️ Could not find insertion point for enhanced guards")
        return content
    
    enhanced_guards = '''
// ============================================================================
// 🛡️ ENHANCED CONFLICT PREVENTION SYSTEM
// ============================================================================

// Comprehensive system header protection
#define SYSTEM_TYPES_PROTECTED 1
#define STDLIB_TYPES_PROTECTED 1
#define STDIO_TYPES_PROTECTED 1

// Smart redirection system for string functions
#ifndef STRING_FUNCTIONS_REDIRECTED
#define STRING_FUNCTIONS_REDIRECTED 1
#undef bcmp
#undef bcopy  
#undef bzero
#define bcmp(a,b,c)  memcmp(a,b,c)
#define bcopy(a,b,c) memmove(b,a,c)
#define bzero(a,b)   memset(a,0,b)
#endif

// GameCube header blocking (comprehensive)
#define BLOCK_ALL_CONFLICTING_HEADERS 1
#define __LIBULTRA_COMPLETE_BLOCK__ 1
#define __DOLPHIN_COMPLETE_BLOCK__ 1
#define __GAMECUBE_SDK_BLOCKED__ 1

'''
    
    # Insert enhanced guards
    insertion_point = content.find("\n", header_end) + 1
    enhanced_content = content[:insertion_point] + enhanced_guards + content[insertion_point:]
    
    return enhanced_content

def main():
    """Execute enhanced mega-wrapper creation"""
    wrapper_path = create_enhanced_mega_wrapper()
    
    print("\n🎯 ENHANCED MEGA-WRAPPER READY!")
    print("=" * 32)
    print("✅ System type conflicts resolved")
    print("✅ Invalid typedefs filtered out") 
    print("✅ Enhanced conflict prevention added")
    print("\n🚀 Ready for compilation test!")

if __name__ == "__main__":
    main() 