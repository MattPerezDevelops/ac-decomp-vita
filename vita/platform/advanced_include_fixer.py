#!/usr/bin/env python3
"""
🎯 Advanced Include Order Fixer
Ensures our types are visible to AC-Decomp headers and removes conflicting declarations
"""

import os
import re
import glob

class AdvancedIncludeFixer:
    def __init__(self):
        self.platform_header = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h'
        self.platform_impl = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c'
        self.ac_decomp_include_dir = '/Users/matt/RiderProjects/ac-decomp/ac-decomp-upstream/include'
        
    def fix_all_include_issues(self):
        """Fix all include order and declaration conflict issues"""
        
        print("🎯 ADVANCED INCLUDE ORDER FIXING")
        print("Solving Gfx visibility and declaration conflicts\n")
        
        fixes = 0
        fixes += self._ensure_early_type_visibility()
        fixes += self._remove_conflicting_external_declarations()
        fixes += self._fix_function_signature_conflicts()
        fixes += self._add_missing_types_from_ac_decomp()
        fixes += self._patch_critical_ac_headers()
        
        print(f"\n✅ Applied {fixes} advanced include fixes")
        return fixes
    
    def _ensure_early_type_visibility(self) -> int:
        """Ensure our essential types are visible before any AC-Decomp includes"""
        
        print("🔧 Ensuring early type visibility...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Create a minimal types-only header section at the very top
        essential_types_early = '''#ifndef AC_VITA_PLATFORM_H
#define AC_VITA_PLATFORM_H

// ===================================================================
// ESSENTIAL TYPES - MUST BE DEFINED BEFORE ANY AC-DECOMP INCLUDES
// ===================================================================

// Basic GameCube types (required by AC-Decomp headers)
#ifndef GFX_DEFINED
#define GFX_DEFINED
typedef struct { u32 cmd[2]; } Gfx;
#endif

#ifndef GSETIMG_DEFINED  
#define GSETIMG_DEFINED
typedef struct { u32 cmd; void* data; } Gsetimg;
#endif

#ifndef LIGHT_T_DEFINED
#define LIGHT_T_DEFINED  
typedef struct { u8 r, g, b; s8 x, y, z; } Light_t;
#endif

// ===================================================================
// END ESSENTIAL TYPES
// ===================================================================

'''
        
        # If we don't have this structure, rebuild the header
        if 'ESSENTIAL TYPES - MUST BE DEFINED BEFORE' not in content:
            # Remove old header guard if it exists
            content = re.sub(r'#ifndef AC_VITA_PLATFORM_H.*?#define AC_VITA_PLATFORM_H\s*\n', '', content, flags=re.DOTALL)
            
            # Add the essential types at the very beginning
            content = essential_types_early + content
            
            with open(self.platform_header, 'w') as f:
                f.write(content)
            
            print("   ✅ Added essential types at header beginning")
            return 1
        
        return 0
    
    def _remove_conflicting_external_declarations(self) -> int:
        """Remove our placeholder declarations that conflict with AC-Decomp's real ones"""
        
        print("🔧 Removing conflicting external declarations...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Functions that AC-Decomp declares differently - remove our versions
        conflicting_externals = [
            'ucode_GetSpriteTextStart',
            'ucode_GetSpriteDataStart'
        ]
        
        fixes = 0
        for func_name in conflicting_externals:
            # Remove our extern declaration
            pattern = rf'extern [^;]*{func_name}[^;]*;'
            if re.search(pattern, content):
                content = re.sub(pattern, f'// {func_name} declared in AC-Decomp headers', content)
                fixes += 1
                print(f"   ➖ Removed conflicting extern: {func_name}")
        
        # Also remove conflicting constants that AC-Decomp defines differently
        conflicting_constants = [
            'G_CYC_COPY',
            'TEV_COMBINED', 
            'TEV_ENVIRONMENT'
        ]
        
        for const_name in conflicting_constants:
            pattern = rf'#define {const_name}[^\n]*\n'
            if re.search(pattern, content):
                content = re.sub(pattern, f'// {const_name} defined in AC-Decomp headers\n', content)
                fixes += 1
                print(f"   ➖ Removed conflicting constant: {const_name}")
        
        if fixes > 0:
            with open(self.platform_header, 'w') as f:
                f.write(content)
        
        return fixes
    
    def _fix_function_signature_conflicts(self) -> int:
        """Fix function signature conflicts between our placeholders and AC-Decomp"""
        
        print("🔧 Fixing function signature conflicts...")
        
        with open(self.platform_impl, 'r') as f:
            impl_content = f.read()
        
        # Remove implementations of functions that AC-Decomp declares differently
        conflicting_implementations = [
            'ucode_GetSpriteTextStart',
            'ucode_GetSpriteDataStart'
        ]
        
        fixes = 0
        for func_name in conflicting_implementations:
            # Remove our implementation
            pattern = rf'[^/\n]*{func_name}\([^{{]*\{{[^}}]*\}}'
            if re.search(pattern, impl_content, re.DOTALL):
                impl_content = re.sub(pattern, f'// {func_name} implemented in AC-Decomp', impl_content, flags=re.DOTALL)
                fixes += 1
                print(f"   ➖ Removed conflicting implementation: {func_name}")
        
        if fixes > 0:
            with open(self.platform_impl, 'w') as f:
                f.write(impl_content)
        
        return fixes
    
    def _add_missing_types_from_ac_decomp(self) -> int:
        """Add types that AC-Decomp expects but we haven't defined"""
        
        print("🔧 Adding missing types from AC-Decomp analysis...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Types that AC-Decomp headers reference but we need to define
        missing_ac_types = '''
// Additional types required by AC-Decomp headers
#ifndef UOBJBG_DEFINED
#define UOBJBG_DEFINED
typedef struct { u32 data[32]; } uObjBg;
#endif

'''
        
        if 'UOBJBG_DEFINED' not in content:
            # Add after essential types section
            insertion_point = content.find('// END ESSENTIAL TYPES')
            if insertion_point != -1:
                end_point = content.find('\n', insertion_point) + 1
                content = content[:end_point] + missing_ac_types + content[end_point:]
                
                with open(self.platform_header, 'w') as f:
                    f.write(content)
                
                print("   ➕ Added missing AC-Decomp types")
                return 1
        
        return 0
    
    def _patch_critical_ac_headers(self) -> int:
        """Patch critical AC-Decomp headers to include our types"""
        
        print("🔧 Patching critical AC-Decomp headers...")
        
        # Key headers that need our types
        critical_headers = [
            'libultra/libultra.h',
            'dolphin/gx.h', 
            'PR/gs2dex.h'
        ]
        
        fixes = 0
        for header_path in critical_headers:
            full_path = os.path.join(self.ac_decomp_include_dir, header_path)
            if os.path.exists(full_path):
                with open(full_path, 'r') as f:
                    header_content = f.read()
                
                # Add our platform header include at the very top if not present
                platform_include = '#include "../../../vita/platform/ac_vita_platform.h"\n'
                
                if platform_include.strip() not in header_content:
                    # Add after initial header guard or at the very beginning
                    if '#ifndef' in header_content[:200]:
                        # Find the line after the header guard
                        lines = header_content.split('\n')
                        insert_idx = 2  # After #ifndef and #define
                        lines.insert(insert_idx, platform_include.strip())
                        header_content = '\n'.join(lines)
                    else:
                        header_content = platform_include + header_content
                    
                    with open(full_path, 'w') as f:
                        f.write(header_content)
                    
                    print(f"   🔧 Patched {header_path}")
                    fixes += 1
        
        return fixes
    
    def test_include_fixes(self) -> bool:
        """Test if our include fixes resolved the issues"""
        
        print("\n🧪 Testing include order fixes...")
        
        import subprocess
        
        cmd = [
            "docker", "run", "--rm", 
            "-v", f"{os.getcwd()}/..:/workspace",
            "-w", "/workspace/vita",
            "ac-vita-unified",
            "bash", "-c", "cd build && timeout 90 make 2>&1 | grep -E '(unknown type name|conflicting types|Building.*\\.c\\.obj)' | head -15"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        output = result.stdout + result.stderr
        
        unknown_types = len([line for line in output.split('\n') if 'unknown type name' in line])
        conflicting_types = len([line for line in output.split('\n') if 'conflicting types' in line])
        building_files = len([line for line in output.split('\n') if 'Building' in line and '.c.obj' in line])
        
        print(f"   📊 Unknown types: {unknown_types}")
        print(f"   📊 Conflicting types: {conflicting_types}")
        print(f"   📊 Files building: {building_files}")
        
        return unknown_types < 3 and building_files > 2

def main():
    print("🎯 ADVANCED INCLUDE ORDER FIXER")
    print("Solving visibility and conflict issues systematically\n")
    
    fixer = AdvancedIncludeFixer()
    fixes = fixer.fix_all_include_issues()
    
    if fixes > 0:
        success = fixer.test_include_fixes()
        if success:
            print("✅ Include fixes successful! Ready for massive build!")
        else:
            print("⚠️  More fixes needed. Ready for next iteration.")
    else:
        print("ℹ️  No include fixes needed.")

if __name__ == "__main__":
    main() 