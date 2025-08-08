#!/usr/bin/env python3
"""
AC-Decomp to Vita Source Converter
Automatically converts AC-Decomp source files to use the Vita platform wrapper

This tool:
1. Replaces all GameCube SDK includes with ac_vita_platform.h
2. Converts asset #include statements to runtime loading
3. Adds initialization calls where needed
4. Preserves original code structure and comments
"""

import os
import re
import json
import argparse
from pathlib import Path

class ACSourceConverter:
    def __init__(self):
        self.asset_mapping = {}
        self.conversion_stats = {
            'files_processed': 0,
            'includes_replaced': 0,
            'assets_converted': 0,
            'functions_wrapped': 0
        }
    
    def load_asset_mapping(self, mapping_file):
        """Load the asset mapping JSON file"""
        try:
            with open(mapping_file, 'r') as f:
                self.asset_mapping = json.load(f)
            print(f"📦 Loaded {len(self.asset_mapping)} asset mappings")
        except Exception as e:
            print(f"⚠️ Could not load asset mapping: {e}")
            self.asset_mapping = {}
    
    def convert_file(self, input_path, output_path):
        """Convert a single AC-Decomp source file to use Vita wrapper"""
        print(f"🔄 Converting {input_path} -> {output_path}")
        
        try:
            with open(input_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"❌ Error reading {input_path}: {e}")
            return False
        
        # Apply all conversions
        converted_content = self.convert_includes(content)
        converted_content = self.convert_asset_includes(converted_content)
        converted_content = self.convert_function_calls(converted_content)
        converted_content = self.add_vita_initialization(converted_content)
        
        # Ensure output directory exists
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        try:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(converted_content)
            self.conversion_stats['files_processed'] += 1
            return True
        except Exception as e:
            print(f"❌ Error writing {output_path}: {e}")
            return False
    
    def convert_includes(self, content):
        """Replace GameCube SDK includes with Vita platform wrapper"""
        
        # GameCube SDK headers that should be replaced
        gc_headers = [
            'dolphin/os.h', 'dolphin/gx.h', 'dolphin/pad.h', 'dolphin/ax.h',
            'dolphin/card.h', 'dolphin/dvd.h', 'dolphin/vi.h', 'dolphin/ai.h',
            'dolphin/dsp.h', 'dolphin/si.h', 'dolphin/exi.h',
            'gx.h', 'os.h', 'pad.h', 'ax.h', 'card.h', 'dvd.h'
        ]
        
        # Track if we need to add our wrapper include
        needs_wrapper = False
        lines = content.split('\n')
        new_lines = []
        wrapper_added = False
        
        for line in lines:
            # Check for GameCube SDK includes
            include_match = re.search(r'#include\\s*[<"](.*?)[>"]', line)
            if include_match:
                header = include_match.group(1)
                
                # Replace GameCube headers
                if any(gc_header in header for gc_header in gc_headers):
                    if not wrapper_added:
                        new_lines.append('#include "ac_vita_platform.h"  // Vita platform wrapper')
                        wrapper_added = True
                        needs_wrapper = True
                        self.conversion_stats['includes_replaced'] += 1
                    # Comment out the original include
                    new_lines.append(f'// {line}  // Replaced by ac_vita_platform.h')
                    continue
            
            new_lines.append(line)
        
        return '\n'.join(new_lines)
    
    def convert_asset_includes(self, content):
        """Convert asset #include statements to runtime loading"""
        
        # Pattern to match asset array declarations with includes
        asset_pattern = re.compile(
            r'(static\\s+)?(\\w+)\\s+(\\w+)\\[\\]\\s*(?:ATTRIBUTE_ALIGN\\(\\d+\\))?\\s*=\\s*{\\s*#include\\s*["\']assets/([^"\']+)["\']\\s*};',
            re.MULTILINE | re.DOTALL
        )
        
        def replace_asset_include(match):
            static_keyword = match.group(1) or ''
            data_type = match.group(2)
            var_name = match.group(3)
            asset_path = match.group(4)
            
            # Convert to runtime loading
            replacement = f'''// Asset: {asset_path} -> Runtime Loading
{static_keyword}{data_type}* {var_name} = NULL;

// Auto-loader for {var_name}
__attribute__((constructor))
static void _load_{var_name}(void) {{
    // Load asset at runtime using asset bridge
    {var_name} = ({data_type}*)ac_get_asset_data("{var_name}");
    if (!{var_name}) {{
        printf("⚠️ Failed to load asset: {var_name}\\n");
    }}
}}'''
            
            self.conversion_stats['assets_converted'] += 1
            return replacement
        
        return asset_pattern.sub(replace_asset_include, content)
    
    def convert_function_calls(self, content):
        """Convert specific function calls that need special handling"""
        
        # Function call mappings
        function_mappings = {
            # Graphics initialization
            'GXInit': 'AC_Vita_Platform_Init',
            
            # Main loop functions
            'VIWaitForRetrace': 'AC_Vita_Frame_End',
            
            # Memory functions that need size adjustments
            'MEMAllocFromExpHeap': 'OSAllocFromHeap',
            'MEMFreeToExpHeap': 'OSFreeToHeap',
        }
        
        converted = content
        for old_func, new_func in function_mappings.items():
            # Simple function name replacement
            old_pattern = rf'\\b{re.escape(old_func)}\\b'
            if re.search(old_pattern, converted):
                converted = re.sub(old_pattern, new_func, converted)
                self.conversion_stats['functions_wrapped'] += 1
        
        return converted
    
    def add_vita_initialization(self, content):
        """Add Vita-specific initialization where needed"""
        
        # Look for main() function and add initialization
        main_pattern = re.compile(r'(int\\s+main\\s*\\([^)]*\\)\\s*{)', re.MULTILINE)
        
        def add_init_to_main(match):
            main_signature = match.group(1)
            
            init_code = f'''{main_signature}
    // Vita Platform Initialization
    if (!AC_Vita_Platform_Init()) {{
        return -1;
    }}
    
    // Register cleanup handler
    atexit(AC_Vita_Platform_Cleanup);
    
'''
            return init_code
        
        # Only add if main() exists and doesn't already have our init
        if 'int main(' in content and 'AC_Vita_Platform_Init' not in content:
            content = main_pattern.sub(add_init_to_main, content)
        
        return content
    
    def convert_directory(self, input_dir, output_dir, recursive=True):
        """Convert all source files in a directory"""
        input_path = Path(input_dir)
        output_path = Path(output_dir)
        
        if not input_path.exists():
            print(f"❌ Input directory does not exist: {input_dir}")
            return False
        
        # File extensions to process
        source_extensions = {'.c', '.cpp', '.h', '.hpp'}
        
        files_to_process = []
        if recursive:
            for ext in source_extensions:
                files_to_process.extend(input_path.rglob(f'*{ext}'))
        else:
            for ext in source_extensions:
                files_to_process.extend(input_path.glob(f'*{ext}'))
        
        print(f"📁 Found {len(files_to_process)} source files to convert")
        
        success_count = 0
        for file_path in files_to_process:
            # Calculate relative path and output location
            rel_path = file_path.relative_to(input_path)
            out_path = output_path / rel_path
            
            if self.convert_file(file_path, out_path):
                success_count += 1
        
        print(f"✅ Successfully converted {success_count}/{len(files_to_process)} files")
        return success_count == len(files_to_process)
    
    def print_stats(self):
        """Print conversion statistics"""
        print("\n📊 Conversion Statistics:")
        print(f"   📄 Files processed: {self.conversion_stats['files_processed']}")
        print(f"   📦 Includes replaced: {self.conversion_stats['includes_replaced']}")
        print(f"   🎨 Assets converted: {self.conversion_stats['assets_converted']}")
        print(f"   🔧 Functions wrapped: {self.conversion_stats['functions_wrapped']}")


def main():
    parser = argparse.ArgumentParser(description='Convert AC-Decomp source files for Vita')
    parser.add_argument('input', help='Input directory or file')
    parser.add_argument('output', help='Output directory or file')
    parser.add_argument('--mapping', help='Asset mapping JSON file', 
                       default='ac_asset_mapping.json')
    parser.add_argument('--recursive', '-r', action='store_true', 
                       help='Process directories recursively')
    parser.add_argument('--single', '-s', action='store_true',
                       help='Process a single file instead of directory')
    
    args = parser.parse_args()
    
    print("🔄 AC-Decomp to Vita Source Converter")
    print("====================================")
    
    converter = ACSourceConverter()
    
    # Load asset mapping if available
    if os.path.exists(args.mapping):
        converter.load_asset_mapping(args.mapping)
    
    # Convert files
    if args.single:
        success = converter.convert_file(Path(args.input), Path(args.output))
    else:
        success = converter.convert_directory(args.input, args.output, args.recursive)
    
    # Print results
    converter.print_stats()
    
    if success:
        print("🎉 Conversion completed successfully!")
        return 0
    else:
        print("❌ Conversion completed with errors")
        return 1


if __name__ == '__main__':
    exit(main()) 