#!/usr/bin/env python3
"""
Fix all Gfx** function signatures to Gfx* to match AC-Decomp usage patterns
"""

import re

def fix_gfx_signatures():
    # Files to fix
    files = [
        '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h',
        '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c'
    ]
    
    for filepath in files:
        print(f"Fixing {filepath}...")
        
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Fix function declarations/definitions that use Gfx**
        # Pattern: functions that take Gfx** as first argument
        patterns = [
            (r'(\w+)\(Gfx\*\* gfx([^)]*)\)', r'\1(Gfx* gfx\2)'),
            (r'(\w+)\(Gfx\*\*\s+gfx([^)]*)\)', r'\1(Gfx* gfx\2)'),
        ]
        
        changes = 0
        for pattern, replacement in patterns:
            new_content, count = re.subn(pattern, replacement, content)
            content = new_content
            changes += count
            
        # Fix function calls that increment gfx pointer
        # Replace (*gfx)++ with (void)gfx since we changed to Gfx*
        increment_patterns = [
            (r'if \(gfx\) \(\*gfx\)\+\+;', r'(void)gfx;'),
            (r'\(\*gfx\)\+\+;', r'(void)gfx;'),
        ]
        
        for pattern, replacement in increment_patterns:
            new_content, count = re.subn(pattern, replacement, content)
            content = new_content
            changes += count
        
        if changes > 0:
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"  Made {changes} changes")
        else:
            print(f"  No changes needed")

if __name__ == "__main__":
    print("🔧 Fixing Gfx** → Gfx* function signatures...")
    fix_gfx_signatures()
    print("✅ Done!") 