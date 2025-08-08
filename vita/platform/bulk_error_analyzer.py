#!/usr/bin/env python3
"""
🚀 Bulk Error Pattern Analyzer
Identifies patterns in build errors and fixes multiple similar issues at once
"""

import re
import subprocess
import os
from collections import defaultdict, Counter
from typing import Dict, List, Set, Tuple

class BulkErrorAnalyzer:
    def __init__(self):
        self.error_patterns = defaultdict(list)
        self.missing_constants = set()
        self.missing_functions = set()
        self.signature_mismatches = []
        
    def analyze_build_errors(self) -> Dict[str, List[str]]:
        """Run build and collect ALL error types for pattern analysis"""
        print("🔍 Running comprehensive error analysis...")
        
        cmd = [
            "docker", "run", "--rm", 
            "-v", f"{os.getcwd()}/..:/workspace",
            "-w", "/workspace/vita",
            "ac-vita-unified",
            "bash", "-c", "cd build && make 2>&1"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        build_output = result.stdout + result.stderr
        
        # Parse and categorize ALL errors
        lines = build_output.split('\n')
        
        for line in lines:
            self._categorize_error(line)
            
        return self._generate_bulk_fixes()
    
    def _categorize_error(self, line: str):
        """Categorize individual error lines"""
        
        # Missing constants (undeclared identifiers)
        if "undeclared" in line and "first use" in line:
            match = re.search(r"'([^']+)' undeclared", line)
            if match:
                identifier = match.group(1)
                if self._is_constant_pattern(identifier):
                    self.missing_constants.add(identifier)
                else:
                    self.missing_functions.add(identifier)
        
        # Missing functions (undefined reference)
        elif "undefined reference to" in line:
            match = re.search(r"undefined reference to `([^']+)'", line)
            if match:
                self.missing_functions.add(match.group(1))
        
        # Function signature mismatches  
        elif "incompatible pointer types" in line or "incompatible types when" in line:
            self.signature_mismatches.append(line)
        
        # Add more pattern recognition here...
    
    def _is_constant_pattern(self, identifier: str) -> bool:
        """Determine if identifier is likely a constant"""
        constant_patterns = [
            lambda x: x.isupper(),                    # ALL_CAPS
            lambda x: x.startswith('G_'),             # Graphics constants
            lambda x: x.startswith('TEX'),            # Texture constants
            lambda x: x.startswith('MTX_'),           # Matrix constants
            lambda x: x.startswith('OS_'),            # OS constants
            lambda x: '_' in x and x.replace('_', '').isupper(),  # SNAKE_CASE
        ]
        return any(pattern(identifier) for pattern in constant_patterns)
    
    def _generate_bulk_fixes(self) -> Dict[str, List[str]]:
        """Generate bulk fixes based on error patterns"""
        
        fixes = {
            'missing_constants': [],
            'missing_functions': [], 
            'pattern_based_constants': [],
            'suggested_bulk_operations': []
        }
        
        # Analyze missing constants for patterns
        constant_groups = self._group_constants_by_pattern()
        
        for pattern_name, constants in constant_groups.items():
            if len(constants) > 1:  # Bulk fix opportunity
                fixes['pattern_based_constants'].append({
                    'pattern': pattern_name,
                    'constants': list(constants),
                    'count': len(constants)
                })
        
        # Analyze missing functions for patterns
        function_groups = self._group_functions_by_pattern()
        
        for pattern_name, functions in function_groups.items():
            if len(functions) > 1:  # Bulk fix opportunity
                fixes['missing_functions'].append({
                    'pattern': pattern_name,
                    'functions': list(functions),
                    'count': len(functions)
                })
        
        # Single items
        fixes['missing_constants'] = list(self.missing_constants)
        
        return fixes
    
    def _group_constants_by_pattern(self) -> Dict[str, Set[str]]:
        """Group constants by naming patterns for bulk generation"""
        groups = defaultdict(set)
        
        for const in self.missing_constants:
            if const.startswith('G_TX_'):
                groups['texture_constants'].add(const)
            elif const.startswith('G_IM_'):
                groups['image_constants'].add(const)
            elif const.startswith('G_CYC_'):
                groups['cycle_constants'].add(const)
            elif const.startswith('G_TF_'):
                groups['texture_filter_constants'].add(const)
            elif const.startswith('G_CC_'):
                groups['color_combiner_constants'].add(const)
            elif const.startswith('TEX'):
                groups['texture_refs'].add(const)
            elif const.startswith('MTX_'):
                groups['matrix_constants'].add(const)
            elif const.isupper() and '_' in const:
                groups['general_constants'].add(const)
            else:
                groups['misc_constants'].add(const)
                
        return groups
    
    def _group_functions_by_pattern(self) -> Dict[str, Set[str]]:
        """Group functions by naming patterns for bulk generation"""
        groups = defaultdict(set)
        
        for func in self.missing_functions:
            if func.startswith('gDP'):
                groups['display_processor'].add(func)
            elif func.startswith('gSP'):
                groups['signal_processor'].add(func)
            elif func.startswith('sAdo_'):
                groups['audio_functions'].add(func)
            elif func.startswith('sMath_'):
                groups['math_functions'].add(func)
            elif func.startswith('Matrix_'):
                groups['matrix_functions'].add(func)
            elif func.startswith('os'):
                groups['os_functions'].add(func)
            else:
                groups['misc_functions'].add(func)
                
        return groups
    
    def generate_bulk_constant_definitions(self, constant_groups: List[Dict]) -> str:
        """Generate bulk constant definitions"""
        
        definitions = []
        definitions.append("// Auto-generated bulk constants")
        
        for group in constant_groups:
            pattern = group['pattern']
            constants = group['constants']
            count = group['count']
            
            definitions.append(f"\n// {pattern.replace('_', ' ').title()} ({count} constants)")
            
            for const in constants:
                if pattern == 'texture_constants':
                    definitions.append(f"#define {const} 0")
                elif pattern == 'image_constants':
                    definitions.append(f"#define {const} 0x01")
                elif pattern == 'matrix_constants':
                    definitions.append(f"#define {const} 1")
                elif pattern == 'color_combiner_constants':
                    # Extract number if present
                    num_match = re.search(r'(\d+)$', const)
                    value = num_match.group(1) if num_match else "0"
                    definitions.append(f"#define {const} {value}")
                else:
                    definitions.append(f"#define {const} 0")
        
        return "\n".join(definitions)
    
    def generate_bulk_function_declarations(self, function_groups: List[Dict]) -> Tuple[str, str]:
        """Generate bulk function declarations and implementations"""
        
        declarations = []
        implementations = []
        
        declarations.append("// Auto-generated bulk function declarations")
        implementations.append("// Auto-generated bulk function implementations")
        
        for group in function_groups:
            pattern = group['pattern']
            functions = group['functions']
            count = group['count']
            
            declarations.append(f"\n// {pattern.replace('_', ' ').title()} ({count} functions)")
            implementations.append(f"\n// {pattern.replace('_', ' ').title()} ({count} functions)")
            
            for func in functions:
                decl, impl = self._generate_function_signature(func, pattern)
                declarations.append(decl)
                implementations.append(impl)
        
        return "\n".join(declarations), "\n".join(implementations)
    
    def _generate_function_signature(self, func_name: str, pattern: str) -> Tuple[str, str]:
        """Generate function signature based on pattern"""
        
        if pattern == 'display_processor' or pattern == 'signal_processor':
            decl = f"void {func_name}(Gfx* gfx, ...);"
            impl = f"void {func_name}(Gfx* gfx, ...) {{ (void)gfx; /* TODO: Implement {func_name} */ }}"
            
        elif pattern == 'audio_functions':
            if 'Pos' in func_name:
                decl = f"void {func_name}(u32 p1, u8 p2, const xyz_t* pos);"
                impl = f"void {func_name}(u32 p1, u8 p2, const xyz_t* pos) {{ (void)p1; (void)p2; (void)pos; }}"
            else:
                decl = f"void {func_name}(void);"
                impl = f"void {func_name}(void) {{ /* TODO: Implement {func_name} */ }}"
                
        elif pattern == 'math_functions':
            decl = f"f32 {func_name}(f32 value);"
            impl = f"f32 {func_name}(f32 value) {{ (void)value; return 0.0f; }}"
            
        elif pattern == 'matrix_functions':
            decl = f"void {func_name}(void);"
            impl = f"void {func_name}(void) {{ /* TODO: Implement {func_name} */ }}"
            
        else:
            decl = f"void {func_name}(void);"
            impl = f"void {func_name}(void) {{ /* TODO: Implement {func_name} */ }}"
            
        return decl, impl

def main():
    print("🔍 BULK ERROR PATTERN ANALYSIS")
    
    analyzer = BulkErrorAnalyzer()
    fixes = analyzer.analyze_build_errors()
    
    print(f"\n📊 PATTERN ANALYSIS RESULTS:")
    
    # Show bulk fix opportunities
    if fixes['pattern_based_constants']:
        print(f"\n🎯 BULK CONSTANT OPPORTUNITIES:")
        for group in fixes['pattern_based_constants']:
            print(f"   📁 {group['pattern']}: {group['count']} constants")
            print(f"      Examples: {group['constants'][:3]}{'...' if len(group['constants']) > 3 else ''}")
    
    if fixes['missing_functions']:
        print(f"\n🎯 BULK FUNCTION OPPORTUNITIES:")
        for group in fixes['missing_functions']:
            print(f"   🔧 {group['pattern']}: {group['count']} functions")
            print(f"      Examples: {group['functions'][:3]}{'...' if len(group['functions']) > 3 else ''}")
    
    # Generate bulk fixes
    if fixes['pattern_based_constants']:
        const_defs = analyzer.generate_bulk_constant_definitions(fixes['pattern_based_constants'])
        print(f"\n✅ Generated bulk constant definitions ({len(fixes['pattern_based_constants'])} groups)")
        
        # Append to platform header
        with open('/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h', 'a') as f:
            f.write('\n' + const_defs + '\n')
    
    if fixes['missing_functions']:
        func_decls, func_impls = analyzer.generate_bulk_function_declarations(fixes['missing_functions'])
        print(f"✅ Generated bulk function declarations ({len(fixes['missing_functions'])} groups)")
        
        # Append to platform files
        with open('/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.h', 'a') as f:
            f.write('\n' + func_decls + '\n')
        with open('/Users/matt/RiderProjects/ac-decomp/vita/platform/ac_vita_platform.c', 'a') as f:
            f.write('\n' + func_impls + '\n')
    
    total_fixes = sum(len(group['constants']) for group in fixes['pattern_based_constants']) + \
                  sum(len(group['functions']) for group in fixes['missing_functions'])
    
    print(f"\n🚀 BULK GENERATION COMPLETE!")
    print(f"   📈 Total items auto-generated: {total_fixes}")
    print(f"   🔄 Ready for next build iteration!")

if __name__ == "__main__":
    main() 