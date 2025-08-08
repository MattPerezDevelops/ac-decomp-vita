#!/usr/bin/env python3
"""
🎯 Smart Conflict Resolver
Eliminates duplicate definitions by using conditional guards and prioritizing AC-Decomp's real values
"""

import re
import os

class SmartConflictResolver:
    def __init__(self):
        self.platform_header = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h'
        self.platform_impl = '/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c'
        
    def resolve_all_conflicts(self):
        """Resolve all identified conflicts systematically"""
        
        print("🎯 SMART CONFLICT RESOLUTION")
        print("Eliminating duplicates and using AC-Decomp's real values\n")
        
        fixes = 0
        fixes += self._remove_conflicting_types()
        fixes += self._remove_conflicting_constants()
        fixes += self._add_conditional_guards()
        fixes += self._clean_duplicate_includes()
        
        print(f"\n✅ Applied {fixes} smart conflict resolutions")
        return fixes
    
    def _remove_conflicting_types(self) -> int:
        """Remove our duplicate type definitions when AC-Decomp has them"""
        
        print("🔧 Removing conflicting type definitions...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Types that conflict with AC-Decomp (we should remove our versions)
        conflicting_types = [
            'Gsetimg',
            'Light_t'
        ]
        
        original_content = content
        
        # Remove our conflicting type definitions
        for type_name in conflicting_types:
            # Remove the typedef line
            pattern = rf'typedef struct.*?{type_name};'
            content = re.sub(pattern, f'// {type_name} defined in AC-Decomp headers', content, flags=re.DOTALL)
            
            # Remove comment above if it exists
            pattern = rf'// Missing types identified from.*?\n.*?{type_name}.*?\n'
            content = re.sub(pattern, '', content, flags=re.DOTALL)
            
            print(f"   ➖ Removed conflicting type: {type_name}")
        
        if content != original_content:
            with open(self.platform_header, 'w') as f:
                f.write(content)
            return len(conflicting_types)
        
        return 0
    
    def _remove_conflicting_constants(self) -> int:
        """Remove our placeholder constants when AC-Decomp has the real values"""
        
        print("🔧 Removing conflicting constants...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Constants that conflict with AC-Decomp (use their real values)
        conflicting_constants = [
            'G_BG_COPY',
            'G_OBJRM_ANTIALIAS', 
            'G_OBJRM_BILERP',
            'G_OBJ_RENDERMODE',
            'G_SC_NON_INTERLACE'
        ]
        
        original_content = content
        
        # Remove our placeholder constant definitions
        for const_name in conflicting_constants:
            pattern = rf'#define {const_name}.*?\n'
            content = re.sub(pattern, f'// {const_name} defined in AC-Decomp headers\n', content)
            print(f"   ➖ Removed conflicting constant: {const_name}")
        
        if content != original_content:
            with open(self.platform_header, 'w') as f:
                f.write(content)
            return len(conflicting_constants)
        
        return 0
    
    def _add_conditional_guards(self) -> int:
        """Add conditional guards to prevent future conflicts"""
        
        print("🔧 Adding conditional compilation guards...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Add guards around our type definitions that might conflict
        guards_to_add = [
            ('Mtx', 'typedef s16 Mtx[4][4];'),
            ('MtxF', 'typedef f32 MtxF[4][4];'),
            ('Vtx', 'typedef struct { f32 pos[3]; f32 norm[3]; f32 tc[2]; } Vtx;'),
            ('Gfx', 'typedef struct { u32 cmd[2]; } Gfx;')
        ]
        
        fixes = 0
        for type_name, typedef in guards_to_add:
            if typedef in content and f'#ifndef {type_name.upper()}_DEFINED' not in content:
                # Wrap the typedef in a guard
                guarded_typedef = f'''#ifndef {type_name.upper()}_DEFINED
#define {type_name.upper()}_DEFINED
{typedef}
#endif'''
                
                content = content.replace(typedef, guarded_typedef)
                fixes += 1
                print(f"   🛡️  Added guard for: {type_name}")
        
        if fixes > 0:
            with open(self.platform_header, 'w') as f:
                f.write(content)
        
        return fixes
    
    def _clean_duplicate_includes(self) -> int:
        """Clean up duplicate include situations"""
        
        print("🔧 Cleaning duplicate includes...")
        
        with open(self.platform_header, 'r') as f:
            content = f.read()
        
        # Add include guard at the top if not present
        if '#ifndef AC_VITA_PLATFORM_H' not in content:
            header_guard = '''#ifndef AC_VITA_PLATFORM_H
#define AC_VITA_PLATFORM_H

'''
            
            # Add at the beginning
            content = header_guard + content
            
            # Add closing guard at the end
            content += '\n#endif // AC_VITA_PLATFORM_H\n'
            
            with open(self.platform_header, 'w') as f:
                f.write(content)
            
            print("   🛡️  Added header guard")
            return 1
        
        return 0
    
    def test_conflict_resolution(self):
        """Test if our conflict resolution worked"""
        
        print("\n🧪 Testing conflict resolution...")
        
        cmd = [
            "docker", "run", "--rm", 
            "-v", f"{os.getcwd()}/..:/workspace",
            "-w", "/workspace/vita",
            "ac-vita-unified",
            "bash", "-c", "cd build && timeout 60 make 2>&1 | grep -E '(conflicting types|redefined|Building.*\\.c\\.obj)' | head -10"
        ]
        
        import subprocess
        result = subprocess.run(cmd, capture_output=True, text=True)
        output = result.stdout + result.stderr
        
        conflict_count = len([line for line in output.split('\n') if 'conflicting types' in line or 'redefined' in line])
        building_count = len([line for line in output.split('\n') if 'Building' in line and '.c.obj' in line])
        
        print(f"   📊 Conflicts remaining: {conflict_count}")
        print(f"   📊 Files building: {building_count}")
        
        return conflict_count == 0 and building_count > 0

def main():
    print("🎯 SMART CONFLICT RESOLVER")
    print("Systematically eliminating duplicate definitions\n")
    
    resolver = SmartConflictResolver()
    fixes = resolver.resolve_all_conflicts()
    
    if fixes > 0:
        success = resolver.test_conflict_resolution()
        if success:
            print("✅ Conflict resolution successful! Ready to build all AC-Decomp files!")
        else:
            print("⚠️  More conflicts detected. Ready for next iteration.")
    else:
        print("ℹ️  No conflicts found to resolve.")

if __name__ == "__main__":
    main() 