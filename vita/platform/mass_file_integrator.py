#!/usr/bin/env python3
"""
🚀 AC-Decomp Mass File Integrator
Automatically adds hundreds of AC-Decomp source files to CMakeLists.txt
"""

import os
import re
import glob
from typing import List, Dict, Set

class MassFileIntegrator:
    def __init__(self, ac_decomp_src_path: str, cmake_file: str):
        self.ac_decomp_src = ac_decomp_src_path
        self.cmake_file = cmake_file
        self.problematic_files = set()
        
    def discover_all_source_files(self) -> Dict[str, List[str]]:
        """Discover all AC-Decomp source files organized by category"""
        
        categories = {
            'core_systems': [],
            'effects': [],
            'actors': [],
            'furniture': [],
            'background_items': [],
            'audio': [],
            'game_logic': [],
            'system': [],
            'misc': []
        }
        
        # Find all .c files
        all_files = []
        for root, dirs, files in os.walk(self.ac_decomp_src):
            for file in files:
                if file.endswith('.c'):
                    rel_path = os.path.relpath(os.path.join(root, file), 
                                             os.path.dirname(self.ac_decomp_src))
                    all_files.append(rel_path)
        
        print(f"🔍 Found {len(all_files)} total source files")
        
        # Categorize files
        for file_path in all_files:
            if '/effect/' in file_path:
                categories['effects'].append(file_path)
            elif '/actor/' in file_path:
                categories['actors'].append(file_path)  
            elif '/furniture/' in file_path:
                categories['furniture'].append(file_path)
            elif '/bg_item/' in file_path:
                categories['background_items'].append(file_path)
            elif '/system/' in file_path:
                categories['system'].append(file_path)
            elif any(x in file_path.lower() for x in ['audio', 'sound']):
                categories['audio'].append(file_path)
            elif any(x in file_path.lower() for x in ['game', 'main', 'play']):
                categories['game_logic'].append(file_path)
            elif any(x in file_path.lower() for x in ['keyframe', 'matrix', 'math']):
                categories['core_systems'].append(file_path)
            else:
                categories['misc'].append(file_path)
        
        return categories
    
    def prioritize_files_by_dependencies(self, categories: Dict[str, List[str]]) -> List[str]:
        """Prioritize files based on likely dependency complexity (simple first)"""
        
        priority_order = []
        
        # 1. Core systems (essential building blocks)
        priority_order.extend(categories['core_systems'][:5])
        
        # 2. Simple effects (usually just math and graphics)
        simple_effects = [f for f in categories['effects'] 
                         if not any(x in f.lower() for x in ['complex', 'advanced', 'room'])]
        priority_order.extend(simple_effects[:50])  # Start with 50 effects
        
        # 3. System files (OS abstractions)  
        priority_order.extend(categories['system'][:10])
        
        # 4. Simple actors (NPCs, objects)
        simple_actors = [f for f in categories['actors']
                        if not any(x in f.lower() for x in ['complex', 'npc', 'post'])]
        priority_order.extend(simple_actors[:30])  # 30 simple actors
        
        # 5. Background items
        priority_order.extend(categories['background_items'][:20])
        
        # 6. Core game logic (more complex)
        priority_order.extend(categories['game_logic'][:10])
        
        # 7. Audio systems (complex dependencies)
        priority_order.extend(categories['audio'][:5])
        
        # 8. Furniture (complex asset dependencies)
        simple_furniture = [f for f in categories['furniture'][:20]]
        priority_order.extend(simple_furniture)
        
        # 9. Miscellaneous
        priority_order.extend(categories['misc'][:10])
        
        return priority_order
    
    def detect_problematic_files(self, file_list: List[str]) -> List[str]:
        """Detect files that are likely to have complex dependencies"""
        
        problematic_patterns = [
            'post_office',  # Complex UI systems
            'museum',       # Complex asset systems
            'nook',         # Complex economy systems
            'camera',       # Complex graphics systems
            'flashrom',     # Hardware-specific
            'network',      # Network functionality
            'debug',        # Debug-specific code
            'test',         # Test code
        ]
        
        clean_files = []
        problematic_files = []
        
        for file_path in file_list:
            if any(pattern in file_path.lower() for pattern in problematic_patterns):
                problematic_files.append(file_path)
            else:
                clean_files.append(file_path)
        
        print(f"📊 Filtered out {len(problematic_files)} potentially problematic files")
        return clean_files
    
    def update_cmake_with_files(self, file_list: List[str], batch_size: int = 100) -> None:
        """Update CMakeLists.txt with a batch of source files"""
        
        # Read current CMakeLists.txt
        with open(self.cmake_file, 'r') as f:
            content = f.read()
        
        # Find the AC_CORE_SOURCES section
        sources_pattern = r'(set\(AC_CORE_SOURCES\s*)(.*?)(\s*\))'
        match = re.search(sources_pattern, content, re.DOTALL)
        
        if not match:
            print("❌ Could not find AC_CORE_SOURCES in CMakeLists.txt")
            return
        
        # Take a batch of files
        batch_files = file_list[:batch_size]
        
        # Generate new sources list
        new_sources = []
        new_sources.append("# Core system - proven to work")
        new_sources.append('"../ac-decomp-upstream/src/c_keyframe.c"')
        new_sources.append("")
        
        # Add files by category with comments
        effects = [f for f in batch_files if '/effect/' in f]
        actors = [f for f in batch_files if '/actor/' in f]
        systems = [f for f in batch_files if '/system/' in f]
        others = [f for f in batch_files if f not in effects + actors + systems]
        
        if effects:
            new_sources.append(f"# Effects system ({len(effects)} files)")
            for file_path in effects:
                new_sources.append(f'"{file_path}"')
            new_sources.append("")
        
        if actors:
            new_sources.append(f"# Actor system ({len(actors)} files)")
            for file_path in actors:
                new_sources.append(f'"{file_path}"')
            new_sources.append("")
        
        if systems:
            new_sources.append(f"# System files ({len(systems)} files)")
            for file_path in systems:
                new_sources.append(f'"{file_path}"')
            new_sources.append("")
        
        if others:
            new_sources.append(f"# Other files ({len(others)} files)")
            for file_path in others:
                new_sources.append(f'"{file_path}"')
        
        # Replace the sources section properly  
        new_sources_text = "\n            ".join(new_sources)
        new_content = re.sub(
            sources_pattern, 
            rf'\1\n            {new_sources_text}\n        {match.group(3)}',
            content,
            flags=re.DOTALL
        )
        
        # Write updated CMakeLists.txt
        with open(self.cmake_file, 'w') as f:
            f.write(new_content)
        
        print(f"✅ Added {len(batch_files)} files to CMakeLists.txt")
        print(f"   📁 Effects: {len(effects)}")
        print(f"   🎭 Actors: {len(actors)}")  
        print(f"   ⚙️  Systems: {len(systems)}")
        print(f"   📄 Others: {len(others)}")

def main():
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python3 mass_file_integrator.py <batch_size>")
        print("Example: python3 mass_file_integrator.py 100")
        return
    
    try:
        batch_size = int(sys.argv[1])
    except ValueError:
        print("❌ Batch size must be a number")
        return
    
    if batch_size > 500:
        print("⚠️  Warning: Batch sizes over 500 may cause very long build times")
        response = input("Continue? (y/N): ")
        if response.lower() != 'y':
            return
    
    # Paths
    ac_decomp_src = "/Users/matt/RiderProjects/ac-decomp/ac-decomp-upstream/src"
    cmake_file = "/Users/matt/RiderProjects/ac-decomp/vita/CMakeLists.txt"
    
    integrator = MassFileIntegrator(ac_decomp_src, cmake_file)
    
    print(f"🚀 MASS FILE INTEGRATION - BATCH SIZE: {batch_size}")
    
    # Discover all files
    categories = integrator.discover_all_source_files()
    
    # Show statistics
    total_files = sum(len(files) for files in categories.values())
    print(f"\n📊 FILE CATEGORIES:")
    for category, files in categories.items():
        print(f"   {category}: {len(files)} files")
    print(f"   TOTAL: {total_files} files")
    
    # Prioritize and filter
    prioritized_files = integrator.prioritize_files_by_dependencies(categories)
    clean_files = integrator.detect_problematic_files(prioritized_files)
    
    print(f"\n🎯 INTEGRATION PLAN:")
    print(f"   📋 Total prioritized: {len(prioritized_files)}")
    print(f"   ✅ Clean files: {len(clean_files)}")
    print(f"   🔄 This batch: {min(batch_size, len(clean_files))}")
    
    # Confirm
    response = input(f"\n🤔 Add {min(batch_size, len(clean_files))} files to build? (y/N): ")
    if response.lower() != 'y':
        print("❌ Cancelled")
        return
    
    # Update CMakeLists.txt
    integrator.update_cmake_with_files(clean_files, batch_size)
    
    print(f"\n🎉 SUCCESS! Added {min(batch_size, len(clean_files))} files to build")
    print("🔄 Run the auto wrapper generator to fix any missing functions")
    print("🏗️  Then run the build to test compilation")

if __name__ == "__main__":
    main() 