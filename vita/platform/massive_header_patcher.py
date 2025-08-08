#!/usr/bin/env python3
"""
🎯 Massive Header Patcher
Systematically patches ALL AC-Decomp headers to include our platform types
"""

import os
import glob
import re

class MassiveHeaderPatcher:
    def __init__(self):
        self.ac_decomp_include_dir = '/Users/matt/RiderProjects/ac-decomp/ac-decomp-upstream/include'
        self.platform_include = '#include "../../../vita/platform/ac_vita_platform.h"'
        
    def patch_all_headers(self):
        """Patch all AC-Decomp headers that reference our types"""
        
        print("🎯 MASSIVE HEADER PATCHING")
        print("Adding platform includes to ALL AC-Decomp headers that need our types\n")
        
        total_patches = 0
        total_patches += self._patch_headers_with_types()
        total_patches += self._patch_critical_system_headers()
        total_patches += self._patch_headers_by_pattern()
        
        print(f"\n✅ Applied {total_patches} header patches")
        return total_patches
    
    def _patch_headers_with_types(self) -> int:
        """Patch headers that directly reference our types"""
        
        print("🔧 Patching headers with direct type references...")
        
        # Types that require our platform header
        our_types = ['Gfx', 'Mtx', 'MtxF', 'Vtx', 'GXTexObj', 'PadStatus', 'Light_t', 'Gsetimg']
        
        patches = 0
        
        # Search all .h files for our types
        for root, dirs, files in os.walk(self.ac_decomp_include_dir):
            for file in files:
                if file.endswith('.h'):
                    file_path = os.path.join(root, file)
                    
                    try:
                        with open(file_path, 'r') as f:
                            content = f.read()
                        
                        # Check if this header references our types
                        needs_patch = any(type_name in content for type_name in our_types)
                        already_patched = self.platform_include in content
                        
                        if needs_patch and not already_patched:
                            patched_content = self._add_platform_include(content, file_path)
                            
                            with open(file_path, 'w') as f:
                                f.write(patched_content)
                            
                            rel_path = os.path.relpath(file_path, self.ac_decomp_include_dir)
                            print(f"   🔧 Patched {rel_path}")
                            patches += 1
                    
                    except (UnicodeDecodeError, PermissionError):
                        # Skip binary or protected files
                        continue
        
        return patches
    
    def _patch_critical_system_headers(self) -> int:
        """Patch critical system headers that other files depend on"""
        
        print("🔧 Patching critical system headers...")
        
        critical_headers = [
            'libultra/libultra.h',
            'dolphin/gx.h',
            'dolphin/types.h', 
            'PR/gbi.h',
            'PR/gs2dex.h',
            'THA_GA.h',
            'sys_matrix.h',
            'padmgr.h',
            'libforest/gbi_extensions.h'
        ]
        
        patches = 0
        for header_path in critical_headers:
            full_path = os.path.join(self.ac_decomp_include_dir, header_path)
            if os.path.exists(full_path):
                patches += self._patch_single_header(full_path, header_path)
        
        return patches
    
    def _patch_headers_by_pattern(self) -> int:
        """Patch headers based on filename patterns that likely need our types"""
        
        print("🔧 Patching headers by pattern...")
        
        # Patterns that likely need graphics/system types
        patterns = [
            '**/gfx*.h',
            '**/render*.h', 
            '**/effect*.h',
            '**/actor*.h',
            '**/bg_*.h',
            '**/game*.h'
        ]
        
        patches = 0
        for pattern in patterns:
            pattern_path = os.path.join(self.ac_decomp_include_dir, pattern)
            for file_path in glob.glob(pattern_path, recursive=True):
                if file_path.endswith('.h'):
                    rel_path = os.path.relpath(file_path, self.ac_decomp_include_dir)
                    patches += self._patch_single_header(file_path, rel_path)
        
        return patches
    
    def _patch_single_header(self, file_path: str, rel_path: str) -> int:
        """Patch a single header file"""
        
        try:
            with open(file_path, 'r') as f:
                content = f.read()
            
            if self.platform_include not in content:
                patched_content = self._add_platform_include(content, file_path)
                
                with open(file_path, 'w') as f:
                    f.write(patched_content)
                
                print(f"   🔧 Patched {rel_path}")
                return 1
        
        except (UnicodeDecodeError, PermissionError):
            pass
        
        return 0
    
    def _add_platform_include(self, content: str, file_path: str) -> str:
        """Add platform include to header content in the right place"""
        
        lines = content.split('\n')
        
        # Find the best insertion point
        insert_idx = 0
        
        # Look for header guard
        for i, line in enumerate(lines):
            if line.strip().startswith('#ifndef') and i < 10:
                # Insert after header guard definition
                insert_idx = i + 2
                break
            elif line.strip().startswith('#define') and '_H' in line and i < 10:
                insert_idx = i + 1
                break
        
        # If no header guard found, insert at the beginning
        if insert_idx == 0:
            insert_idx = 0
        
        # Calculate relative path for the include
        file_dir = os.path.dirname(file_path)
        relative_depth = file_path.replace(self.ac_decomp_include_dir, '').count('/')
        platform_path = '../' * relative_depth + '../vita/platform/ac_vita_platform.h'
        platform_include_line = f'#include "{platform_path}"'
        
        # Insert the include
        lines.insert(insert_idx, platform_include_line)
        lines.insert(insert_idx + 1, '')  # Add blank line
        
        return '\n'.join(lines)
    
    def test_header_patches(self) -> bool:
        """Test if our header patches resolved the type issues"""
        
        print("\n🧪 Testing header patches...")
        
        import subprocess
        
        cmd = [
            "docker", "run", "--rm", 
            "-v", f"{os.getcwd()}/..:/workspace",
            "-w", "/workspace/vita",
            "ac-vita-unified",
            "bash", "-c", "cd build && timeout 120 make 2>&1 | grep -E '(unknown type name|Building.*\\.c\\.obj|Linking)' | head -20"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        output = result.stdout + result.stderr
        
        unknown_types = len([line for line in output.split('\n') if 'unknown type name' in line])
        building_files = len([line for line in output.split('\n') if 'Building' in line and '.c.obj' in line])
        linking_detected = 'Linking' in output
        
        print(f"   📊 Unknown types: {unknown_types}")
        print(f"   📊 Files building: {building_files}")
        print(f"   📊 Linking stage: {'YES' if linking_detected else 'NO'}")
        
        if linking_detected:
            print("   🎉 REACHED LINKING STAGE!")
            return True
        elif building_files > 3 and unknown_types < 5:
            print("   🚀 Good progress - continuing compilation!")
            return True
        else:
            return False

def main():
    print("🎯 MASSIVE HEADER PATCHER")
    print("Ensuring ALL AC-Decomp headers can see our platform types\n")
    
    patcher = MassiveHeaderPatcher()
    patches = patcher.patch_all_headers()
    
    if patches > 0:
        success = patcher.test_header_patches()
        if success:
            print("✅ Header patches successful! Ready for massive compilation!")
        else:
            print("⚠️  More patches needed. Ready for next iteration.")
    else:
        print("ℹ️  Headers already patched.")

if __name__ == "__main__":
    main() 