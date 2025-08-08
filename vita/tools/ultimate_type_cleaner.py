#!/usr/bin/env python3

"""
Ultimate Type Cleaner for AC Vita Port
=====================================

Removes all system type conflicts and auto-generated placeholders 
from gx_to_vitagl_wrapper.h to enable AC-decomp mass compilation.

Critical Issues Fixed:
- ldiv_t, lldiv_t, va_list system conflicts
- acosf math function conflicts  
- 1000+ auto-generated typedef u32 placeholders
- Invalid numeric typedefs
"""

import re
import os
import sys

def clean_wrapper_types(wrapper_path):
    """Clean all problematic types from the wrapper file"""
    
    print("🧹 ULTIMATE TYPE CLEANER")
    print("========================")
    print(f"Cleaning: {wrapper_path}")
    
    with open(wrapper_path, 'r') as f:
        content = f.read()
    
    original_lines = len(content.split('\n'))
    
    # PHASE 1: Remove system type conflicts
    print("\n📋 PHASE 1: Removing system type conflicts...")
    
    system_types = [
        'ldiv_t', 'lldiv_t', 'va_list', 'FILE', 'size_t', 'wchar_t',
        'div_t', 'fpos_t', 'time_t', 'clock_t', 'ptrdiff_t'
    ]
    
    for sys_type in system_types:
        pattern = rf'^typedef\s+\w+\s+{sys_type}\s*;.*$'
        old_count = len(re.findall(pattern, content, re.MULTILINE))
        content = re.sub(pattern, '', content, flags=re.MULTILINE)
        if old_count > 0:
            print(f"  ✅ Removed {old_count} conflicting {sys_type} typedef(s)")
    
    # PHASE 2: Remove math function conflicts
    print("\n📋 PHASE 2: Removing math function conflicts...")
    
    math_functions = [
        'acosf', 'asinf', 'atanf', 'cosf', 'sinf', 'tanf', 'sqrtf',
        'powf', 'logf', 'expf', 'floorf', 'ceilf', 'fabsf'
    ]
    
    for math_func in math_functions:
        pattern = rf'^.*{math_func}.*Auto-generated.*$'
        old_count = len(re.findall(pattern, content, re.MULTILINE))
        content = re.sub(pattern, '', content, flags=re.MULTILINE)
        if old_count > 0:
            print(f"  ✅ Removed {old_count} conflicting {math_func} declaration(s)")
    
    # PHASE 3: Remove auto-generated placeholders
    print("\n📋 PHASE 3: Removing auto-generated placeholders...")
    
    # Remove all "typedef u32 NAME; // Auto-generated placeholder" lines
    placeholder_pattern = r'^typedef\s+u32\s+\w+\s*;\s*//\s*Auto-generated placeholder.*$'
    placeholder_count = len(re.findall(placeholder_pattern, content, re.MULTILINE))
    content = re.sub(placeholder_pattern, '', content, flags=re.MULTILINE)
    print(f"  ✅ Removed {placeholder_count} auto-generated typedef u32 placeholders")
    
    # PHASE 4: Remove invalid numeric typedefs
    print("\n📋 PHASE 4: Removing invalid numeric typedefs...")
    
    # Remove lines like "typedef u32 1234;"
    numeric_pattern = r'^typedef\s+\w+\s+\d+\s*;.*$'
    numeric_count = len(re.findall(numeric_pattern, content, re.MULTILINE))
    content = re.sub(numeric_pattern, '', content, flags=re.MULTILINE)
    if numeric_count > 0:
        print(f"  ✅ Removed {numeric_count} invalid numeric typedef(s)")
    
    # PHASE 5: Remove duplicate empty lines
    print("\n📋 PHASE 5: Cleaning up formatting...")
    
    # Replace multiple consecutive empty lines with single empty line
    content = re.sub(r'\n\s*\n\s*\n+', '\n\n', content)
    
    # PHASE 6: Add enhanced conflict prevention
    print("\n📋 PHASE 6: Adding enhanced conflict prevention...")
    
    # Add system type guards at the top after includes
    conflict_guards = """
// Enhanced System Type Conflict Prevention
#ifndef _SYSTEM_TYPES_GUARDED
#define _SYSTEM_TYPES_GUARDED

// Block system types that conflict with VitaSDK
#define ldiv_t     vita_ldiv_t_blocked
#define lldiv_t    vita_lldiv_t_blocked  
#define va_list    vita_va_list_blocked
#define FILE       vita_FILE_blocked
#define div_t      vita_div_t_blocked
#define fpos_t     vita_fpos_t_blocked

// Block math functions that conflict with VitaSDK
#define acosf      vita_acosf_blocked
#define asinf      vita_asinf_blocked
#define atanf      vita_atanf_blocked

#endif // _SYSTEM_TYPES_GUARDED
"""
    
    # Insert after the first #include but before any typedefs
    include_end = content.find('#include', content.find('#include') + 1)
    if include_end == -1:
        include_end = content.find('\n', content.find('#include')) + 1
    else:
        include_end = content.find('\n', include_end) + 1
    
    content = content[:include_end] + conflict_guards + content[include_end:]
    
    # Write cleaned content
    with open(wrapper_path, 'w') as f:
        f.write(content)
    
    final_lines = len(content.split('\n'))
    
    print(f"\n✅ CLEANING COMPLETE!")
    print(f"========================")
    print(f"Lines before: {original_lines}")
    print(f"Lines after:  {final_lines}")
    print(f"Lines removed: {original_lines - final_lines}")
    print(f"Reduction: {((original_lines - final_lines) / original_lines * 100):.1f}%")
    
    return True

def test_compilation(build_dir):
    """Test compilation after cleaning"""
    
    print("\n🔨 TESTING COMPILATION AFTER CLEANUP")
    print("====================================")
    
    os.chdir(build_dir)
    
    # Quick compile test
    import subprocess
    result = subprocess.run(['make', '-j2'], capture_output=True, text=True)
    
    # Count errors
    error_count = result.stderr.count('error:')
    warning_count = result.stderr.count('warning:')
    
    print(f"Compilation errors: {error_count}")
    print(f"Compilation warnings: {warning_count}")
    
    if error_count == 0:
        print("🎉 COMPILATION SUCCESS! Type cleanup worked!")
        return True
    else:
        print("🔧 Some errors remain, but major progress made!")
        # Show first few errors
        errors = [line for line in result.stderr.split('\n') if 'error:' in line][:5]
        for error in errors:
            print(f"  {error}")
        return False

if __name__ == "__main__":
    wrapper_path = "../platform/gx_to_vitagl_wrapper.h"
    build_dir = "../build"
    
    if not os.path.exists(wrapper_path):
        print(f"❌ Error: Wrapper file not found: {wrapper_path}")
        sys.exit(1)
    
    # Clean the wrapper
    success = clean_wrapper_types(wrapper_path)
    
    if success:
        print("\n🚀 TYPE CLEANUP COMPLETE!")
        print("========================")
        print("Ready for AC-decomp mass compilation test!")
    else:
        print("❌ Cleanup failed!")
        sys.exit(1) 