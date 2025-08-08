#!/usr/bin/env python3
"""
🚀 AC-Decomp Vita Auto Wrapper Generator
Automatically analyzes compilation errors and generates missing wrapper functions
"""

import re
import os
import subprocess
import sys
from typing import List, Dict, Set, Tuple

class AutoWrapperGenerator:
    def __init__(self, platform_header_path: str, platform_impl_path: str):
        self.platform_header = platform_header_path
        self.platform_impl = platform_impl_path
        self.missing_functions = set()
        self.missing_constants = set()
        self.missing_types = set()
        
    def analyze_build_errors(self) -> Dict[str, List[str]]:
        """Run build and analyze all compilation errors"""
        print("🔍 Analyzing build errors...")
        
        # Run build in Docker and capture errors
        cmd = [
            "docker", "run", "--rm", 
            "-v", f"{os.getcwd()}/..:/workspace",
            "-w", "/workspace/vita",
            "ac-vita-unified",
            "bash", "-c", "cd build && make 2>&1"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        build_output = result.stdout + result.stderr
        
        errors = {
            'undefined_functions': [],
            'undefined_constants': [],
            'undefined_types': [],
            'conflicting_types': []
        }
        
        # Parse error patterns
        for line in build_output.split('\n'):
            # Undefined reference errors (linking stage)
            if "undefined reference to" in line:
                match = re.search(r"undefined reference to `([^']+)'", line)
                if match:
                    func_name = match.group(1)
                    errors['undefined_functions'].append(func_name)
                    
            # Undeclared identifier errors (compilation stage)  
            elif "undeclared" in line and "first use" in line:
                match = re.search(r"'([^']+)' undeclared", line)
                if match:
                    identifier = match.group(1)
                    if identifier.isupper() or identifier.startswith('G_'):
                        errors['undefined_constants'].append(identifier)
                    else:
                        errors['undefined_functions'].append(identifier)
                        
            # Unknown type name errors
            elif "unknown type name" in line:
                match = re.search(r"unknown type name '([^']+)'", line)
                if match:
                    type_name = match.group(1)
                    errors['undefined_types'].append(type_name)
                    
            # Conflicting types (need to check signatures)
            elif "conflicting types for" in line:
                match = re.search(r"conflicting types for '([^']+)'", line)
                if match:
                    func_name = match.group(1)
                    errors['conflicting_types'].append(func_name)
        
        # Remove duplicates
        for key in errors:
            errors[key] = list(set(errors[key]))
            
        return errors
    
    def generate_function_declaration(self, func_name: str) -> str:
        """Generate a reasonable function declaration based on naming patterns"""
        
        # Audio functions
        if func_name.startswith('sAdo_'):
            if 'Pos' in func_name:
                return f"void {func_name}(u32 p1, u8 p2, const xyz_t* pos);"
            elif 'Trg' in func_name:
                return f"void {func_name}(u16 id, const xyz_t* pos);"
            else:
                return f"void {func_name}(void);"
                
        # Math functions
        elif func_name.startswith('sMath_'):
            if 'Rotate' in func_name:
                return f"void {func_name}(xyz_t* v, f32 angle);"
            else:
                return f"f32 {func_name}(f32 value);"
                
        # Matrix functions
        elif func_name.startswith('Matrix_'):
            return f"void {func_name}(void);"
            
        # Memory functions
        elif 'alloc' in func_name.lower() or 'free' in func_name.lower():
            if 'free' in func_name.lower():
                return f"void {func_name}(void* ptr);"
            else:
                return f"void* {func_name}(size_t size);"
                
        # Default: void function with no parameters
        else:
            return f"void {func_name}(void);"
    
    def generate_function_implementation(self, func_name: str) -> str:
        """Generate a placeholder implementation"""
        
        # Get the declaration to extract return type
        decl = self.generate_function_declaration(func_name)
        
        if decl.startswith('void'):
            return f"""void {func_name}({self._extract_params(decl)}) {{
    // TODO: Implement {func_name}
    {self._generate_param_suppressions(decl)}
}}"""
        elif decl.startswith('f32'):
            return f"""f32 {func_name}({self._extract_params(decl)}) {{
    // TODO: Implement {func_name}  
    {self._generate_param_suppressions(decl)}
    return 0.0f;
}}"""
        elif decl.startswith('void*'):
            return f"""void* {func_name}({self._extract_params(decl)}) {{
    // TODO: Implement {func_name}
    {self._generate_param_suppressions(decl)}
    return NULL;
}}"""
        else:
            return f"""int {func_name}({self._extract_params(decl)}) {{
    // TODO: Implement {func_name}
    {self._generate_param_suppressions(decl)}
    return 0;
}}"""
    
    def _extract_params(self, declaration: str) -> str:
        """Extract parameter list from function declaration"""
        match = re.search(r'\(([^)]*)\)', declaration)
        if match:
            return match.group(1)
        return "void"
    
    def _generate_param_suppressions(self, declaration: str) -> str:
        """Generate (void)param; lines to suppress unused warnings"""
        params = self._extract_params(declaration)
        if params == "void" or not params.strip():
            return ""
            
        suppressions = []
        for param in params.split(','):
            param = param.strip()
            if param and param != "void":
                # Extract parameter name (last word before optional array brackets)
                param_name = re.search(r'(\w+)(?:\[.*\])?$', param)
                if param_name:
                    suppressions.append(f"(void){param_name.group(1)};")
                    
        return '\n    '.join(suppressions)
    
    def generate_constant_definition(self, const_name: str) -> str:
        """Generate constant definitions based on naming patterns"""
        
        if const_name.startswith('G_SPECIAL_'):
            # Extract number from name
            match = re.search(r'G_SPECIAL_(\d+)', const_name)
            if match:
                return f"#define {const_name} {match.group(1)}"
            else:
                return f"#define {const_name} 0"
                
        elif const_name.startswith('G_'):
            return f"#define {const_name} 0x01"
            
        elif const_name.startswith('MTX_'):
            return f"#define {const_name} 1"
            
        elif const_name.startswith('OS_'):
            return f"#define {const_name} 0"
            
        else:
            return f"#define {const_name} 0"
    
    def generate_type_definition(self, type_name: str) -> str:
        """Generate type definitions based on naming patterns"""
        
        if type_name.startswith('OS'):
            return f"typedef void* {type_name};"
        elif type_name.endswith('_t'):
            return f"typedef struct {type_name} {type_name};"
        else:
            return f"typedef void* {type_name};"
    
    def update_platform_files(self, errors: Dict[str, List[str]]) -> int:
        """Update platform header and implementation files with missing items"""
        
        total_additions = 0
        
        # Read current files
        with open(self.platform_header, 'r') as f:
            header_content = f.read()
        with open(self.platform_impl, 'r') as f:
            impl_content = f.read()
        
        # Add missing constants
        new_constants = []
        for const in errors['undefined_constants']:
            if f"#define {const}" not in header_content:
                new_constants.append(self.generate_constant_definition(const))
                total_additions += 1
        
        if new_constants:
            header_content += "\n// Auto-generated constants\n" + "\n".join(new_constants) + "\n"
        
        # Add missing types
        new_types = []
        for type_name in errors['undefined_types']:
            if f"typedef" not in header_content or type_name not in header_content:
                new_types.append(self.generate_type_definition(type_name))
                total_additions += 1
        
        if new_types:
            header_content += "\n// Auto-generated types\n" + "\n".join(new_types) + "\n"
        
        # Add missing function declarations and implementations
        new_declarations = []
        new_implementations = []
        
        for func in errors['undefined_functions']:
            if f"{func}(" not in header_content:
                decl = self.generate_function_declaration(func)
                impl = self.generate_function_implementation(func)
                
                new_declarations.append(decl)
                new_implementations.append(impl)
                total_additions += 1
        
        if new_declarations:
            header_content += "\n// Auto-generated function declarations\n" + "\n".join(new_declarations) + "\n"
            impl_content += "\n// Auto-generated function implementations\n" + "\n\n".join(new_implementations) + "\n"
        
        # Write updated files
        with open(self.platform_header, 'w') as f:
            f.write(header_content)
        with open(self.platform_impl, 'w') as f:
            f.write(impl_content)
            
        return total_additions

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--dry-run":
        dry_run = True
        print("🔍 DRY RUN MODE - Will analyze but not modify files")
    else:
        dry_run = False
        print("🚀 ACTIVE MODE - Will analyze and update wrapper files")
    
    # Paths
    platform_header = "/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h"
    platform_impl = "/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c"
    
    generator = AutoWrapperGenerator(platform_header, platform_impl)
    
    # Analyze errors
    errors = generator.analyze_build_errors()
    
    print(f"\n📊 ANALYSIS RESULTS:")
    print(f"   🔴 Missing functions: {len(errors['undefined_functions'])}")
    print(f"   🟡 Missing constants: {len(errors['undefined_constants'])}")
    print(f"   🟠 Missing types: {len(errors['undefined_types'])}")
    print(f"   🔵 Conflicting types: {len(errors['conflicting_types'])}")
    
    if errors['undefined_functions']:
        print(f"\n🔍 Missing functions: {errors['undefined_functions'][:10]}{'...' if len(errors['undefined_functions']) > 10 else ''}")
    if errors['undefined_constants']:
        print(f"🔍 Missing constants: {errors['undefined_constants'][:10]}{'...' if len(errors['undefined_constants']) > 10 else ''}")
    if errors['undefined_types']:
        print(f"🔍 Missing types: {errors['undefined_types'][:5]}{'...' if len(errors['undefined_types']) > 5 else ''}")
    
    if not dry_run:
        # Update files
        additions = generator.update_platform_files(errors)
        print(f"\n✅ AUTO-GENERATED {additions} missing items!")
        print("🔄 Run the build again to see progress!")
    else:
        print(f"\n💡 Would auto-generate ~{len(errors['undefined_functions']) + len(errors['undefined_constants']) + len(errors['undefined_types'])} items")

if __name__ == "__main__":
    main() 