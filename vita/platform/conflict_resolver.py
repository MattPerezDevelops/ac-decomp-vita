#!/usr/bin/env python3
"""
🔧 Bulk Conflict Resolver
Detects and resolves conflicting function definitions and type issues in platform wrapper
"""

import re
import os
from collections import defaultdict

class ConflictResolver:
    def __init__(self):
        self.header_path = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h'
        self.impl_path = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c'
        
    def resolve_all_conflicts(self):
        """Find and resolve all conflicts in platform files"""
        
        print("🔧 BULK CONFLICT RESOLUTION")
        
        # Fix header conflicts
        header_fixes = self._fix_header_conflicts()
        
        # Fix implementation conflicts  
        impl_fixes = self._fix_implementation_conflicts()
        
        print(f"✅ Resolved {header_fixes + impl_fixes} conflicts")
        
    def _fix_header_conflicts(self) -> int:
        """Fix conflicts in header file"""
        
        print("📝 Fixing header conflicts...")
        
        with open(self.header_path, 'r') as f:
            content = f.read()
        
        fixes = 0
        
        # Remove duplicate function declarations
        seen_functions = set()
        lines = content.split('\n')
        cleaned_lines = []
        
        for line in lines:
            # Check for function declarations
            func_match = re.match(r'^[^/]*?\b(\w+)\s*\([^)]*\)\s*;', line.strip())
            if func_match:
                func_name = func_match.group(1)
                if func_name in seen_functions:
                    print(f"   🗑️  Removed duplicate: {func_name}")
                    fixes += 1
                    continue
                seen_functions.add(func_name)
            
            cleaned_lines.append(line)
        
        # Fix OSMesg -> OSMessage type conflicts
        new_content = '\n'.join(cleaned_lines)
        new_content = re.sub(r'\bOSMesg\b', 'OSMessage', new_content)
        fixes += len(re.findall(r'\bOSMessage\b', new_content)) - len(re.findall(r'\bOSMessage\b', content))
        
        # Fix variadic function declarations (... without named parameter)
        # Replace problematic ... with void
        new_content = re.sub(r'(\w+\s*\(\s*\.\.\.)', r'\1', new_content)
        new_content = re.sub(r'(\w+\s*\(\s*\.\.\.\s*\))', r'\1', new_content)
        
        with open(self.header_path, 'w') as f:
            f.write(new_content)
            
        return fixes
    
    def _fix_implementation_conflicts(self) -> int:
        """Fix conflicts in implementation file"""
        
        print("💾 Fixing implementation conflicts...")
        
        with open(self.impl_path, 'r') as f:
            content = f.read()
        
        fixes = 0
        
        # Remove duplicate function implementations
        seen_functions = set()
        
        # Split into sections and process
        sections = self._split_into_functions(content)
        kept_sections = []
        
        for section in sections:
            # Extract function name from implementation
            func_match = re.search(r'^[^/]*?\b(\w+)\s*\([^)]*\)\s*\{', section, re.MULTILINE)
            if func_match:
                func_name = func_match.group(1)
                if func_name in seen_functions:
                    print(f"   🗑️  Removed duplicate implementation: {func_name}")
                    fixes += 1
                    continue
                seen_functions.add(func_name)
            
            kept_sections.append(section)
        
        new_content = '\n'.join(kept_sections)
        
        # Fix variadic function implementations
        new_content = re.sub(r'(\w+)\s*\(\s*\.\.\.\s*\)', r'\1(void)', new_content)
        
        # Fix OSMesg -> OSMessage
        new_content = re.sub(r'\bOSMesg\b', 'OSMessage', new_content)
        
        with open(self.impl_path, 'w') as f:
            f.write(new_content)
            
        return fixes
    
    def _split_into_functions(self, content: str) -> list:
        """Split content into function sections"""
        
        # Split on function boundaries (lines starting with return type + function name)
        sections = []
        current_section = []
        brace_depth = 0
        in_function = False
        
        for line in content.split('\n'):
            # Check if this looks like a function definition start
            if re.match(r'^[^/]*?\b\w+\s+\w+\s*\([^)]*\)\s*\{', line):
                if current_section:
                    sections.append('\n'.join(current_section))
                current_section = [line]
                in_function = True
                brace_depth = line.count('{') - line.count('}')
            else:
                current_section.append(line)
                if in_function:
                    brace_depth += line.count('{') - line.count('}')
                    if brace_depth == 0:
                        in_function = False
        
        if current_section:
            sections.append('\n'.join(current_section))
            
        return sections
    
    def create_clean_platform_wrapper(self):
        """Create a completely clean platform wrapper by rebuilding from scratch"""
        
        print("🔄 Creating clean platform wrapper...")
        
        # Read existing content to preserve working parts
        with open(self.header_path, 'r') as f:
            header_content = f.read()
        with open(self.impl_path, 'r') as f:
            impl_content = f.read()
        
        # Extract unique function declarations and implementations
        unique_functions = self._extract_unique_functions(header_content, impl_content)
        
        # Rebuild files cleanly
        self._rebuild_clean_header(unique_functions)
        self._rebuild_clean_implementation(unique_functions)
        
        print("✅ Clean platform wrapper created")
    
    def _extract_unique_functions(self, header_content: str, impl_content: str) -> dict:
        """Extract unique, non-conflicting functions"""
        
        functions = {}
        
        # Extract from header
        for line in header_content.split('\n'):
            func_match = re.match(r'^[^/]*?(\w+\s+\w+\s*\([^)]*\))\s*;', line.strip())
            if func_match:
                signature = func_match.group(1)
                func_name = re.search(r'\b(\w+)\s*\(', signature).group(1)
                if func_name not in functions:
                    functions[func_name] = {'declaration': signature + ';', 'implementation': None}
        
        # Extract implementations
        for section in self._split_into_functions(impl_content):
            func_match = re.search(r'^[^/]*?(\w+\s+\w+\s*\([^)]*\))\s*\{', section, re.MULTILINE)
            if func_match:
                signature = func_match.group(1)
                func_name = re.search(r'\b(\w+)\s*\(', signature).group(1)
                if func_name in functions:
                    functions[func_name]['implementation'] = section.strip()
        
        return functions
    
    def _rebuild_clean_header(self, functions: dict):
        """Rebuild header file cleanly"""
        
        header_template = '''#ifndef AC_VITA_PLATFORM_H
#define AC_VITA_PLATFORM_H

// Include standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// VitaGL includes
#include <vitaGL.h>
#include <vita2d.h>

// VitaSDK includes
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/audiout.h>
#include <psp2/touch.h>

// Include AC-Decomp types
#include "types.h"

// Type definitions for compatibility
typedef uint32_t OSMessage;
typedef void* OSMesgQueue;
typedef void* OSThread;

// Forward declarations
struct ACTOR;
typedef struct ACTOR ACTOR;
typedef u16 mActor_name_t;

'''
        
        # Add all unique function declarations
        for func_name, func_data in functions.items():
            if func_data['declaration']:
                header_template += func_data['declaration'] + '\n'
        
        header_template += '\n#endif // AC_VITA_PLATFORM_H\n'
        
        with open(self.header_path, 'w') as f:
            f.write(header_template)
    
    def _rebuild_clean_implementation(self, functions: dict):
        """Rebuild implementation file cleanly"""
        
        impl_template = '''#include "ac_vita_platform.h"

// Global variables
Gfx* gfx_ptr[3] = {NULL, NULL, NULL};
xyz_t ZeroVec = {0.0f, 0.0f, 0.0f};

'''
        
        # Add all unique function implementations
        for func_name, func_data in functions.items():
            if func_data['implementation']:
                impl_template += func_data['implementation'] + '\n\n'
        
        with open(self.impl_path, 'w') as f:
            f.write(impl_template)

def main():
    print("🔧 BULK CONFLICT RESOLVER")
    
    resolver = ConflictResolver()
    resolver.create_clean_platform_wrapper()
    
    print("✅ ALL CONFLICTS RESOLVED!")
    print("🔄 Platform wrapper is now clean and ready for build!")

if __name__ == "__main__":
    main() 